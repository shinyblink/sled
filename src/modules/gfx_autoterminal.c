// Automatically typing terminal.
//
// Copyright (c) 2019/2020, Sebastian "basxto" Riedel <git@basxto.de>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <matrix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <timers.h>
#include <types.h>

#include "foxel35.xbm"
#include "xbm_font_loader.c"

#include <sys/types.h>
#include <unistd.h>

//#include <pthread.h>
#include <oscore.h>
#include <pty.h>

const int font_width = 4;
const int font_height = 6;

#define FRAMETIME (T_SECOND / 8)
#define FRAMES (TIME_SHORT)

struct font_char {
    unsigned char c;
    RGB fg;
    RGB bg;
    int flags;
};

static oscore_time nexttick;
static int moduleno;
static int max_row;
static int max_column;
static struct font_char *buffer;
static int active_shell = 0;
static char **type_buffer;
static int type_pos;
static int type_index;
static int max_index;
static const RGB fg_default = RGB(247, 127, 190);
static const RGB bg_default = RGB(50, 50, 50);
static RGB fg = fg_default;
static RGB bg = bg_default;
static int current_row = 0;
static int current_column = 0;
static oscore_mutex buffer_busy;
static char *shell;

// scroll buffer up by one line
static void scroll_up() {
    int i;
    for (i = 0; i < ((max_column * (max_row - 1))); ++i) {
        buffer[i].c = buffer[i + max_column].c;
        buffer[i].fg = buffer[i + max_column].fg;
        buffer[i].bg = buffer[i + max_column].bg;
    }
    for (; i < ((max_column * max_row)); ++i) {
        buffer[i].c = ' ';
        buffer[i].fg = fg;
        buffer[i].bg = bg;
    }
}

static RGB sgr2rgb(int code) {
    RGB color = RGB(0, 0, 0);
    int shift = 0;
    // high intensity is 8-15
    if (code > 7 && code < 16) {
        shift = 85;
        code -= 8;
    }
    switch (code % 10) {
    case 0:
        color = RGB(shift, shift, shift);
        break;
    case 1:
        color = RGB(170 + shift, shift, shift);
        break;
    case 2:
        color = RGB(shift, 170 + shift, shift);
        break;
    case 3:
        color = RGB(170 + shift, 85 + shift, shift);
        break;
    case 4:
        color = RGB(shift, shift, 170 + shift);
        break;
    case 5:
        color = RGB(170 + shift, shift, 170 + shift);
        break;
    case 6:
        color = RGB(shift, 170 + shift, 170 + shift);
        break;
    case 7:
        color = RGB(170 + shift, 170 + shift, 170 + shift);
        break;
    }
    if (code >= 16 && code < 232) { /// color cubes
        int cubemap[] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
        code -= 16;
        int r = code / 36;
        int g = (code % 36) / 6;
        int b = code % 6;
        color = RGB(cubemap[r], cubemap[g], cubemap[b]);
    } else if (code >= 232) { // black to white in 24 steps
        int gray = (code - 232) * (256 / 24);
        color = RGB(gray, gray, gray);
    }
    return color;
}

// read decimal number and set i to following character
static int parse_sgr_value(char *str, int i, int *code, int def) {
    int len = strlen(str);
    if (!(i < len && (str[i] >= '0' && str[i] <= '9'))) {
        return def;
    }
    for (*code = 0; i < len && (str[i] >= '0' && str[i] <= '9'); ++i) {
        *code *= 10;
        *code += str[i] - '0';
    }
    return i;
}

// csi sgr
static int interpret_sgr(char *str, int i) {
    int code = 0;
    int len = strlen(str);

    while (i < len) {
        i = parse_sgr_value(str, i, &code, 0);
        if (str[i] == ';' || str[i] == 'm') {
            if (code == 38 || code == 48) {
                RGB color = RGB(0, 0, 0);
                int tmpcode = 0;
                int tmpi;
                tmpi = parse_sgr_value(str, i + 1, &tmpcode, 0);
                i = tmpi;
                if (tmpcode == 5) {
                    i = parse_sgr_value(str, i + 1, &tmpcode, 0);
                    color = sgr2rgb(tmpcode);
                } else if (tmpcode == 2) {
                    // colors are simply given as rgb codes
                    int r;
                    int g;
                    int b;
                    i = parse_sgr_value(str, i + 1, &r, 0);
                    i = parse_sgr_value(str, i + 1, &g, 0);
                    i = parse_sgr_value(str, i + 1, &b, 0);
                    color = RGB(r, g, b);
                }
                if (code < 40)
                    fg = color;
                else
                    bg = color;

            } else if ((code >= 30 && code <= 47) ||
                       (code >= 90 && code <= 107)) {
                // map regular foreground color to 0-7 and high intensity to
                // 8-15
                RGB color = sgr2rgb(code % 10 + (code >= 90 ? 8 : 0));
                if (code < 40)
                    fg = color;
                else
                    bg = color;
            } else
                switch (code) {
                case 0:
                    fg = fg_default;
                    bg = bg_default;
                    break;
                // case 7: //reverse video
                //	break;
                default:
                    printf("Unhandled escape code %d\n", code);
                    break;
                }
            code = 0;
        }
        if (str[i] == 'm') {
            break;
        }
        i++;
    }
    return i;
}

// If string ends with an unfinished escape sequence it
// it returns amount of read characters of that sequence
static int write_buffer(char *str, int *row, int *column) {
    int i;
    int pos = (*column) + ((*row) * max_column);
    int len = strlen(str);
    for (i = 0; i < len; ++i) {
        switch (str[i]) {
        case '\n':
            (*row)++;
            while (*row > max_row) {
                (*row)--;
                scroll_up();
            }
            pos = ((pos / max_column) + 1) * max_column;
            break;
        case '\r':
            *column = 0;
            pos = ((pos / max_column) + 1) * max_column;
            break;
        default:
            if (pos >= 0) {
                buffer[pos].c = str[i];
                buffer[pos].fg = fg;
                buffer[pos].bg = bg;
            }
            (*column)++;
            pos++;
            break;
        }
    }
    // we reached end of string and everything went well
    return 0;
}

static void parse_csi(char *str, int end) {
    int i = 1; // skip [
    int j = 0;
    int j_len = 0;
    int code = 0;
    switch (str[end]) {
    case 'm':
        interpret_sgr(str, i);
        break;
    case 'A':
        i = parse_sgr_value(str, i, &code, 1);
        current_row -= i;
        if (current_row < 0)
            current_row = 0;
        break;
    case 'B':
        i = parse_sgr_value(str, i, &code, 1);
        current_row += i;
        if (current_row > max_row)
            current_row = max_row;
        break;
    case 'C':
        i = parse_sgr_value(str, i, &code, 1);
        current_column += i;
        if (current_column > max_column)
            current_column = max_column;
        break;
    case 'D':
        i = parse_sgr_value(str, i, &code, 1);
        current_column -= i;
        if (current_column < 0)
            current_column = 0;
        break;
    case 'H': // cursor position
        i = parse_sgr_value(str, i, &code, 1);
        current_row = code - 1;
        // column handled by G
    case 'G': // cursor horizontal absolute
        parse_sgr_value(str, i, &code, 1);
        current_column = code - 1;
        break;
    case 'K': // erase line
        parse_sgr_value(str, i, &code, 0);
        switch (code) {
        case 0:
            j = current_column;
            j_len = max_column;
            break;
        case 1:
            j = 0;
            j_len = current_column;
            break;
        case 2:
            j = 0;
            j_len = max_column;
            break;
        }
        for (; j <= j_len; ++j) {
            buffer[j + (current_row * max_column)].c = ' ';
            buffer[j + (current_row * max_column)].fg = fg;
            buffer[j + (current_row * max_column)].bg = bg;
        }
        break;
    case 'J': // erase in display
        parse_sgr_value(str, i, &code, 0);
        switch (code) {
        case 0:
            j = current_column + (current_row * max_column);
            j_len = max_column * max_row;
            break;
        case 1:
            j = 0;
            j_len = current_column + (current_row * max_column);
            break;
        default:
            j = 0;
            j_len = max_column * max_row;
            break;
        }
        for (; j <= j_len; ++j) {
            buffer[j].c = ' ';
            buffer[j].fg = fg;
            buffer[j].bg = bg;
        }
        break;
    default:
        printf("Unhandled CSI type %c\n", str[end]);
    }
}

// run in own thread
// 6* is just a hack, since offset isn’t really working yet
static void *launch(void *type_buffer) {
    char *command = (char *)type_buffer;
    int fd;
    struct winsize win = {max_row, max_column, max_column * font_width,
                          max_row * font_height};
    if (forkpty(&fd, NULL, NULL, &win) == 0) {
        char *args[] = {shell, "-c", command, NULL};
        execv(args[0], args);
    }
    FILE *cout = fdopen(fd, "r");
    // according to man console_codes ESC [ has a maximum of 16 parameters
    // and since 255; is the maximum int value, that makes 16*4
    char *tmpbuffer = malloc(max_column * 6 * sizeof(char));
    int ch;
    int escape_code = 0;
    while ((ch = getc(cout)) != EOF) {
        if (escape_code != 0) {
            tmpbuffer[escape_code - 1] = ch;
            // first char always allowed
            // and all legit characters
            if (escape_code == 1 || (ch >= '0' && ch <= '?')) {
                escape_code++;
            } else { // exit escape code parsing
                tmpbuffer[escape_code] = 0;
                // printf("escape code: %s\n", tmpbuffer);
                if (tmpbuffer[0] == '[')
                    parse_csi(tmpbuffer, escape_code - 1);
                escape_code = 0;
            }
        } else {
            if (ch == 0x1B) {
                escape_code = 1;
            } else {
                tmpbuffer[0] = (unsigned char)ch;
                tmpbuffer[1] = 0;
                oscore_mutex_lock(buffer_busy);
                write_buffer(tmpbuffer, &current_row, &current_column);
                oscore_mutex_unlock(buffer_busy);
            }
        }
    }
    fclose(cout);
    free(tmpbuffer);
    active_shell = 0;
    return 0;
}

static void clear_buffer() {
    int i;
    for (i = 0; i < max_row * max_column; ++i) {
        buffer[i].c = ' ';
        buffer[i].fg = fg;
        buffer[i].bg = bg;
    }
}

int init(int modno, char *argstr) {
    moduleno = modno;
    char *from_int = malloc(10 * sizeof(char));
    max_row = matrix_gety() / 6;
    max_column = matrix_getx() / 4;
    setenv("TERMINFO", "./terminfo/", 1);
    setenv("TERM", "autoterminal", 1);
    snprintf(from_int, 10, "%d", max_row);
    setenv("LINES", from_int, 1);
    snprintf(from_int, 10, "%d", max_column);
    setenv("COLUMNS", from_int, 1);
    max_index = 1;
    shell = malloc(30 * sizeof(char));
    strcpy(shell, "/bin/sh");
    // read script
    FILE *file;

    file = fopen("scripts/autoterminal.sh", "r");
    if (file) {
        // last line endns with eof and not new line
        int ch;
        while ((ch = getc(file)) != EOF)
            if ((unsigned char)ch == '\n')
                max_index++;
        type_buffer = malloc(max_index * sizeof(char *));
        for (type_index = 0; type_index < max_index; type_index++) {
            type_buffer[type_index] = malloc(max_column * 3 * sizeof(char));
        }
        rewind(file);
        int first_line = 1;
        type_index = 0;
        while (fgets(type_buffer[type_index], max_column * 3, file) != NULL) {
            // skip comments
            if (type_buffer[type_index][0] == '#') {
                if (first_line && type_buffer[0][1] == '!') {
                    strcpy(shell, type_buffer[type_index] + 2);
                    // each input ends with a newline
                    int last = strlen(shell) - 1;
                    if (shell[last] == '\n')
                        shell[last] = '\0';
                }
            } else {
                type_index++;
                if (type_index > max_index)
                    break;
            }
            first_line = 0;
        };
        fclose(file);
    }

    type_pos = 0;
    max_index = type_index - 1;
    type_index = 0;

    buffer_busy = oscore_mutex_new();

    buffer = malloc(max_row * max_column * sizeof(struct font_char));
    clear_buffer();

    // 2 rows
    if (matrix_getx() < font_height * 2)
        return 1; // not enough X to be usable
    // 8 characters
    if (matrix_gety() < font_width * 8)
        return 1; // not enough Y to be usable
    return 0;
}

void reset(int _modno) {
    type_index = 0;
    type_pos = 0;
    current_row = 0;
    current_column = 0;
    fg = fg_default;
    bg = bg_default;
    clear_buffer();
    nexttick = udate();
}

int draw(int _modno, int argc, char *argv[]) {
    int x;
    int y;
    int row;
    int column;
    int pos = 0;
    if (active_shell == 0) {
        // we are through with all commands
        if (type_index > max_index)
            return 1;
        // this happens right after other thread finished
        if (type_pos == 0) { // prepare next line or exit
            write_buffer("$ ", &current_row, &current_column);
        }
        if (type_pos < strlen(type_buffer[type_index])) {
            char ch[2];
            ch[0] = type_buffer[type_index][type_pos];
            ch[1] = 0;
            write_buffer(ch, &current_row, &current_column);
            type_pos++;
        } else {
            current_column = 0;
            active_shell = 1;
            type_index++;
            type_pos = 0;
            oscore_task_create("shell", launch, type_buffer[type_index - 1]);
        }
    }

    oscore_mutex_lock(buffer_busy);
    for (row = 0; row < max_row; ++row)
        for (column = 0; column < max_column; ++column) {
            for (y = 0; y < 6; ++y) {
                for (x = 0; x < 4; ++x) {
                    matrix_set((column * 4) + x, (row * 6) + y,
                               (load_char(foxel35_bits, buffer[pos].c, x, y,
                                          font_width, font_height) == 1
                                    ? buffer[pos].fg
                                    : buffer[pos].bg));
                }
            }
            pos++;
        }
    oscore_mutex_unlock(buffer_busy);
    matrix_render();
    nexttick += (FRAMETIME);
    timer_add(nexttick, moduleno, 0, NULL);
    return 0;
}

void deinit(int _modno) {
    free(buffer);
    free(shell);
    for (type_index = 0; type_index < max_index; type_index++) {
        free(type_buffer[type_index]);
    }
    free(type_buffer);
    oscore_mutex_free(buffer_busy);
}
