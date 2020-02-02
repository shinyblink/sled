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
// special font for programs who use half block unicode
#include "microfont.xbm"
#include "printbuffer.h"

#include <sys/types.h>
#include <unistd.h>

#include <oscore.h>
#include <pty.h>
#include <signal.h>

#define FRAMETIME (T_SECOND / 32)
#define TYPEDELAY (4)
#define FRAMES (TIME_SHORT)

#define flag_intense (1)
#define flag_faint (1 << 1)
#define flag_inverse (1 << 2)
#define flag_blink (1 << 3)
#define flag_altchar (1 << 4)
#define fg_default (RGB(255, 255, 255))
#define bg_default (RGB(0, 0, 0))

static int font_width;
static int font_height;
static int font;
static oscore_time nexttick;
static int moduleno;
static int max_row;
static int max_column;
static int type_counter = 0;
// this is accessed by multiple threads
static volatile int active_shell = 0;
static char **type_buffer;
static int type_pos;
static int type_index;
static int max_index;
static RGB fg = fg_default;
static RGB bg = bg_default;
static int current_row = 0;
static int current_column = 0;
static int current_row_store = 0;
static int current_column_store = 0;
static char *shell;
static int flags = 0;
static oscore_task child;
static volatile pid_t grandchild = 0;

static int shift_color(int value, int shift) {
    if (shift < 0) {
        if (value >= -shift) {
            return value + shift;
        } else
            return 0;
    }
    if (shift > 0) {
        if (value + shift < 256) {
            return value + shift;
        } else
            return 255;
    }
    return value;
}

static RGB sgr2rgb(int code, int shift) {
    RGB color = RGB(0, 0, 0);
    // high intensity is 8-15
    if (code > 7 && code < 16) {
        shift += 85;
        code -= 8;
    }
    switch (code % 10) {
    case 0:
        color = RGB(0, 0, 0);
        break;
    case 1:
        color = RGB(170, 0, 0);
        break;
    case 2:
        color = RGB(0, 170, 0);
        break;
    case 3:
        color = RGB(170, 85, 0);
        break;
    case 4:
        color = RGB(0, 0, 170);
        break;
    case 5:
        color = RGB(170, 0, 170);
        break;
    case 6:
        color = RGB(0, 170, 170);
        break;
    case 7:
        color = RGB(170, 170, 170);
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
    color.red = shift_color(color.red, shift);
    color.green = shift_color(color.green, shift);
    color.blue = shift_color(color.blue, shift);
    return color;
}

// read decimal number and set i to following character
static int parse_sgr_value(char *str, int i, int *code, int def) {
    int len = strlen(str);
    if (!(i < len && (str[i] >= '0' && str[i] <= '9'))) {
        *code = def;
        return i;
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
            if (code == 39) { // reset to default color
                if (flags & flag_inverse)
                    bg = fg_default;
                else
                    fg = fg_default;
            } else if (code == 49) {
                if (flags & flag_inverse)
                    fg = bg_default;
                else
                    bg = bg_default;
            } else if (code == 38 || code == 48) { // 8/24bit
                RGB color = RGB(0, 0, 0);
                int tmpcode = 0;
                int tmpi;
                int shift = 0;
                if (flags & flag_intense) {
                    shift += 85;
                }
                if (flags & flag_faint) {
                    shift -= 85;
                }
                // only shift foreground
                if (code >= 40)
                    shift = 0;
                tmpi = parse_sgr_value(str, i + 1, &tmpcode, 0);
                i = tmpi;
                if (tmpcode == 5) {
                    i = parse_sgr_value(str, i + 1, &tmpcode, 0);
                    color = sgr2rgb(tmpcode, shift);
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
                // logic XOR
                if ((code < 40) == !(flags & flag_inverse))
                    fg = color;
                else
                    bg = color;

            } else if ((code >= 30 && code <= 47) ||
                       (code >= 90 && code <= 107)) {
                int shift = 0;
                if (flags & flag_intense) {
                    shift += 85;
                }
                if (flags & flag_faint) {
                    shift -= 85;
                }
                // only shift foreground
                if (code >= 40)
                    shift = 0;
                // map regular foreground color to 0-7 and high intensity to
                // 8-15
                RGB color = sgr2rgb(code % 10 + (code >= 90 ? 8 : 0), shift);
                // translate high color intensity to normal
                if (code >= 90) {
                    code -= 60;
                }

                if ((code < 40) == !(flags & flag_inverse))
                    fg = color;
                else
                    bg = color;
            } else
                switch (code) {
                case 0:
                    fg = fg_default;
                    bg = bg_default;
                    flags &= ~flag_intense;
                    flags &= ~flag_faint;
                    flags &= ~flag_inverse;
                    flags &= ~flag_blink;
                    break;
                case 1:
                    flags |= flag_intense;
                    break;
                case 2:
                    flags |= flag_faint;
                    flags &= ~flag_intense;
                    break;
                case 22:
                    flags &= ~flag_intense;
                    flags &= ~flag_faint;
                    break;
                case 5:
                    flags |= flag_blink;
                    break;
                case 25:
                    flags &= ~flag_blink;
                    break;
                case 7: // reverse video
                    flags |= flag_inverse;
                    break;
                case 27:
                    flags &= ~flag_inverse;
                    break;
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

static void parse_csi(char *str, int end, FILE *cout) {
    int i = 1; // skip [
    int j = 0;
    int j_len = 0;
    int code = 0;
    switch (str[end]) {
    case 'm':
        interpret_sgr(str, i);
        break;
    case 'A':
        parse_sgr_value(str, i, &code, 1);
        current_row -= code;
        if (current_row < 0)
            current_row = 0;
        break;
    case 'B':
        parse_sgr_value(str, i, &code, 1);
        current_row += code;
        if (current_row > max_row)
            current_row = max_row;
        break;
    case 'C':
        parse_sgr_value(str, i, &code, 1);
        current_column += code;
        if (current_column > max_column)
            current_column = max_column;
        break;
    case 'D':
        parse_sgr_value(str, i, &code, 1);
        current_column -= code;
        if (current_column < 0)
            current_column = 0;
        break;
    case 'E':
        parse_sgr_value(str, i, &code, 1);
        current_row += code;
        if (current_row > max_row)
            current_row = max_row;
        current_column = 0;
        break;
    case 'F':
        parse_sgr_value(str, i, &code, 1);
        current_row -= code;
        if (current_row < 0)
            current_row = 0;
        current_column = 0;
        break;
    case 'S': // scroll up
        parse_sgr_value(str, i, &code, 1);
        current_row -= code;
        if (current_row < 0)
            current_row = 0;
        int tmp_row;
        int tmp_column = max_column - 1;
        for (; code > 0; --code) {
            tmp_row = max_row - 1;
            printbuffer_write("\n", &tmp_row, &tmp_column, fg, bg, 0);
        }
        break;
    case 'f':
    case 'H': // cursor position
        i = parse_sgr_value(str, i, &code, 1);
        current_row = code - 1;
        // skip the ;
        if (i < strlen(str))
            i++;
        // column handled by G
    case 'G': // cursor horizontal absolute
        parse_sgr_value(str, i, &code, 1);
        current_column = code - 1;
        break;
    case 'd': // cursor horizontal absolute
        parse_sgr_value(str, i, &code, 1);
        current_row = code - 1;
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
        printbuffer_clear(j + (current_row * max_column),
                          j_len + (current_row * max_column), fg, bg);
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
        printbuffer_clear(j, j_len, fg, bg);
        break;
    case 'u': // restore cursor
        current_row = current_row_store;
        current_column = current_column_store;
        break;
    case 's': // store cursor
        current_row_store = current_row;
        current_column_store = current_column;
        break;
    case 'n':
        parse_sgr_value(str, i, &code, 0);
        if (code == 6) {
            char response[27] = ";;";
            snprintf(response, 27, "\033[%d;%dR", current_row + 1,
                     current_column + 1);
            printf("Type into Terminal: \\E%s\n", response + 1);
            fputs(response, cout);
        } else {
            printf("Invalid h SGR %d\n", code);
        }
        break;
    // and ?25h to show cursor
    // and ?25l to hide cursor
    default:
        printf("Unhandled CSI type %c\n", str[end]);
    }
}

// run in own thread
static void *launch(void *type_buffer) {
    char *command = (char *)type_buffer;
    int fd;
    struct winsize win = {max_row, max_column, max_column * font_width,
                          max_row * font_height};
    grandchild = forkpty(&fd, NULL, NULL, &win);
    if (grandchild == 0) {
        chdir("scripts");
        char *args[] = {shell, "-c", command, NULL};
        execv(args[0], args);
    }
    FILE *cout = fdopen(fd, "r+");
    // according to man console_codes ESC [ has a maximum of 16 parameters
    // and since 255; is the maximum int value, that makes 16*4
    char *tmpbuffer = malloc(16 * 4 * sizeof(char));
    int ch;
    int escape_code = 0;
    int pb_flags;
    while ((ch = getc(cout)) != EOF) {
        if (escape_code != 0) {
            tmpbuffer[escape_code - 1] = ch;
            // first char always allowed
            // and all legit characters
            // everything but [ and ? is followed by just one char
            // (ISO/IEC 2022)
            if (escape_code == 1 || ((ch >= '0' && ch <= '?') &&
                                     !(escape_code > 1 && tmpbuffer[0] != '[' &&
                                       tmpbuffer[0] != '?'))) {
                escape_code++;
            } else { // exit escape code parsing
                tmpbuffer[escape_code] = 0;
                if (tmpbuffer[0] == '[') {
                    parse_csi(tmpbuffer, escape_code - 1, cout);
                } else if (tmpbuffer[0] == '(') {
                    if (tmpbuffer[1] == '0') {
                        flags |= flag_altchar;
                    } else {
                        flags &= !flag_altchar;
                    }
                } else {
                    printf("Unhandled escape code: %s\n", tmpbuffer);
                }
                escape_code = 0;
            }
        } else {
            if (ch == '\a') {
                // ignore bell
            } else if (ch == 0x1B) {
                escape_code = 1;
            } else {
                tmpbuffer[0] = (unsigned char)ch;
                tmpbuffer[1] = 0;
                // detect unicode und forward as one string
                if (tmpbuffer[0] & (1 << (7))) {
                    // go through all bytes; max 3
                    int j;
                    for (j = 0; j < 3 && tmpbuffer[0] & (1 << (6 - j)); ++j) {
                        if ((ch = getc(cout)) == EOF)
                            break;
                        tmpbuffer[1 + j] = (unsigned char)ch;
                        tmpbuffer[2 + j] = 0;
                    }
                }
                // translate flags
                pb_flags = 0;
                if (flags & flag_altchar)
                    pb_flags |= printbuffer_flag_altchar;
                if (flags & flag_blink)
                    pb_flags |= printbuffer_flag_blink;
                printbuffer_write(tmpbuffer, &current_row, &current_column, fg,
                                  bg, pb_flags);
            }
        }
    }
    fclose(cout);
    free(tmpbuffer);
    grandchild = 0;
    active_shell = 0;
    return 0;
}

static int read_script(char *script) {
    FILE *file;
    char name[] = "scripts/autoterminal_XXXXX.sh";
    snprintf(name, strlen(name), "scripts/autoterminal_%s.sh", script);
    file = fopen(name, "r");
    if (file) {
        // last line endns with eof and not new line
        int ch;
        while ((ch = getc(file)) != EOF)
            if ((unsigned char)ch == '\n')
                max_index++;
        type_buffer = malloc(max_index * sizeof(char *));
        for (type_index = 0; type_index < max_index; type_index++) {
            type_buffer[type_index] =
                malloc(MAX(max_column, 40) * 3 * sizeof(char));
        }
        rewind(file);
        int first_line = 1;
        type_index = 0;
        while (fgets(type_buffer[type_index], MAX(max_column, 40) * 3, file)) {
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
        type_pos = 0;
        max_index = type_index - 1;
        type_index = 0;
        return 0;
    } else {
        return 1;
    }
}

int init(int modno, char *argstr) {
    moduleno = modno;
    child = 0;
    grandchild = 0;
    font_width = 4;
    font_height = 6;
    font = 0; // foxel36
    char *from_int = malloc(10 * sizeof(char));
    max_row = matrix_gety() / font_height;
    max_column = matrix_getx() / font_width;
    setenv("TERMINFO", "../terminfo/", 1);
    setenv("TERM", "autoterminal", 1);
    snprintf(from_int, 10, "%d", max_row);
    setenv("LINES", from_int, 1);
    snprintf(from_int, 10, "%d", max_column);
    setenv("COLUMNS", from_int, 1);
    max_index = 1;
    shell = malloc(30 * sizeof(char));
    strcpy(shell, "/bin/sh");
    // read script
    read_script("1");

    printbuffer_init(max_row, max_column, fg_default, bg_default);

    // 2 rows
    if (matrix_getx() < font_height * 2)
        return 1; // not enough X to be usable
    // 8 characters
    if (matrix_gety() < font_width * 8)
        return 1; // not enough Y to be usable
    return 0;
}

void reset(int _modno) {
    if (grandchild) {
        kill(grandchild, SIGTERM);
    }
    grandchild = 0;
    if (child) {
        oscore_task_join(child);
        child = NULL;
    }
    child = 0;
    active_shell = 0;
    type_index = 0;
    type_pos = 0;
    current_row = 0;
    current_column = 0;
    fg = fg_default;
    bg = bg_default;
    printbuffer_clear(0, max_row * max_column, fg, bg);
    // clear unused space
    for (int y = max_row * font_height; y < matrix_gety(); ++y)
        for (int x = 0; x < matrix_getx(); ++x)
            matrix_set(x, y, RGB(0, 0, 0));
    nexttick = udate();
}

int draw(int _modno, int argc, char *argv[]) {
    if (argc > 0) {
        int i;
        for (i = 0; i < argc; ++i) {
            if (argc - i >= 2 && strcmp(argv[i], "script") == 0 &&
                strlen(argv[i + 1]) <= 5) {
                printf("Load script terminal_%s.sh\n", argv[i + 1]);
                read_script(argv[i + 1]);
                ++i;
            } else if (argc - i >= 2 && strcmp(argv[i], "execute") == 0) {
                printf("Execute \"%s\"\n", argv[i + 1]);
                read_script(argv[i + 1]);
                max_index = 0;
                type_index = 0;
                type_buffer = malloc(max_index * sizeof(char *));
                type_buffer[0] =
                    malloc((strlen(argv[i + 1]) + 1) * sizeof(char *));
                strcpy(type_buffer[0], argv[i + 1]);
                ++i;
            } else if (argc - i >= 2 && strcmp(argv[i], "font") == 0) {
                printf("Load font \"%s\"\n", argv[i + 1]);
                if (strcmp(argv[i + 1], "microfont") == 0) {
                    printf("Load microfont!\n");
                    font = 1;
                    font_width = 1;
                    font_height = 2;
                } else {
                    font = 0;
                    font_width = 4;
                    font_height = 6;
                }
                max_row = matrix_gety() / font_height;
                max_column = matrix_getx() / font_width;
                printbuffer_deinit();
                printbuffer_init(max_row, max_column, fg_default, bg_default);
                ++i;
            }
        }
    }
    if (active_shell == 0) {
        if (child) {
            oscore_task_join(child);
            child = NULL;
        }
        if (type_counter <= 0) {
            // we are through with all commands
            if (type_index > max_index)
                return 1;
            // this happens right after other thread finished
            if (type_pos == 0) { // prepare next line or exit
                printbuffer_write("$ ", &current_row, &current_column, fg, bg,
                                  0);
            }
            if (type_pos < strlen(type_buffer[type_index])) {
                char ch[2];
                ch[0] = type_buffer[type_index][type_pos];
                ch[1] = 0;
                printbuffer_write(ch, &current_row, &current_column, fg, bg, 0);
                type_pos++;
            } else {
                current_column = 0;
                active_shell = 1;
                type_index++;
                type_pos = 0;
                child = oscore_task_create("shell", launch,
                                           type_buffer[type_index - 1]);
            }
            type_counter = TYPEDELAY;
        }
        --type_counter;
    }
    unsigned char *font_bits = foxel35_bits;
    if (font == 1)
        font_bits = microfont_bits;
    printbuffer_draw(font_bits, font_width, font_height, 4 * TYPEDELAY);

    matrix_render();
    nexttick += (FRAMETIME);
    timer_add(nexttick, moduleno, 0, NULL);
    return 0;
}

void deinit(int _modno) {
    if (grandchild) {
        kill(grandchild, SIGTERM);
    }
    grandchild = 0;
    if (child) {
        oscore_task_join(child);
        child = NULL;
    }
    child = 0;
    free(shell);
    for (type_index = 0; type_index < max_index; type_index++) {
        free(type_buffer[type_index]);
    }
    free(type_buffer);
    printbuffer_deinit();
}
