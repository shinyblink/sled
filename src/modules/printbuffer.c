// Print strings to an internal buffer and draw that to the matrix.
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

#include "printbuffer.h"
#include <oscore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <matrix.h>
#include "foxel35.xbm"

struct font_char {
    unsigned char c;
    RGB fg;
    RGB bg;
    int flags;
};
static struct font_char *buffer;
static oscore_mutex buffer_busy;
static int max_row;
static int max_column;
static RGB trans = {0,0,0,0};
static int blink;
static int blink_counter;
#define font_width_def 4
#define font_height_def 6

void printbuffer_clear(int from, int to, RGB fg, RGB bg) {
    int i;
    oscore_mutex_lock(buffer_busy);
    for (i = from; i < to; ++i) {
        buffer[i].c = ' ';
        buffer[i].fg = fg;
        buffer[i].bg = bg;
        buffer[i].flags = 0;
    }
    oscore_mutex_unlock(buffer_busy);
}

void printbuffer_clear_default(){
    printbuffer_clear(0, max_row * max_column, trans, trans);
}

void printbuffer_init(int row, int column, RGB fg, RGB bg){
    max_row = row;
    max_column = column;
    blink = 0;
    buffer_busy = oscore_mutex_new();
    buffer = malloc(max_row * max_column * sizeof(struct font_char));

    printbuffer_clear(0, max_row * max_column, fg, bg);
}

void printbuffer_init_default(){
    printbuffer_init(matrix_gety() / font_height_def, matrix_getx() / font_width_def, trans, trans);
}

void printbuffer_deinit(){
    free(buffer);
    oscore_mutex_free(buffer_busy);
}

// scroll buffer up by one line
//buffer must be locked
static void scroll_up(RGB fg, RGB bg) {
    int i;
    for (i = 0; i < ((max_column * (max_row - 1))); ++i) {
        buffer[i].c = buffer[i + max_column].c;
        buffer[i].fg = buffer[i + max_column].fg;
        buffer[i].bg = buffer[i + max_column].bg;
        buffer[i].flags = buffer[i + max_column].flags;
    }
    for (; i < ((max_column * max_row)); ++i) {
        buffer[i].c = ' ';
        buffer[i].fg = fg;
        buffer[i].bg = bg;
        buffer[i].flags = 0;
    }
}

void printbuffer_write(char *str, int *row, int *column, RGB fg, RGB bg, int flags) {
    int i;
    int pos = (*column) + ((*row) * max_column);
    int len = strlen(str);
    oscore_mutex_lock(buffer_busy);
    for (i = 0; i < len; ++i) {
        switch (str[i]) {
        case '\n':
            (*row)++;
            while (*row > max_row) {
                (*row)--;
                scroll_up(fg, bg);
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
                buffer[pos].flags = flags;
            }
            (*column)++;
            pos++;
            break;
        }
    }
    oscore_mutex_unlock(buffer_busy);
    // we reached end of string and everything went well
}

void printbuffer_write_default(char *str, int *row, int *column) {
    printbuffer_write(str, row, column, RGB(0,0,0), trans, 0);
}

// c is the character you want to load
// x and y are pixel inside the character
// xbm must have 16 characters per row
// xbm must contain 128 characters
int load_xbm_char(unsigned char bits[], unsigned char c, int x, int y, int w, int h, int flags) {
    if(flags&printbuffer_flag_altchar){
        c+=128;
    }
    int offset_x = (c % 16) * w;
    int offset_y = (c / 16) * h;
    int total_x = offset_x + x;
    int total_y = offset_y + y;
    int pos = total_y * (w * 16) + total_x;
    return (((bits[pos / 8]) >> ((pos % 8))) & 1);
}


// this calls matrix_set
void printbuffer_draw(unsigned char bits[], int font_width, int font_height, int blink_delay){
    int x;
    int y;
    int row;
    int column;
    int pos = 0;
    int bit;
    RGB color;
    blink_counter--;
    if(blink_counter <= 0){
        blink = !blink;//toggle
        blink_counter = blink_delay;
    }
    oscore_mutex_lock(buffer_busy);
    for (row = 0; row < max_row; ++row)
        for (column = 0; column < max_column; ++column) {
            for (y = 0; y < font_height; ++y) {
                for (x = 0; x < font_width; ++x) {
                    bit = load_xbm_char(bits, buffer[pos].c, x, y, font_width, font_height, buffer[pos].flags);
                    color = (bit == 1 ? buffer[pos].fg : buffer[pos].bg);
                    // invert if blinking
                    if(blink && (buffer[pos].flags&printbuffer_flag_blink)){
                        color = (bit == 1 ? buffer[pos].bg : buffer[pos].fg);
                    }
                    matrix_set((column * 4) + x, (row * 6) + y, color);
                }
            }
            pos++;
        }
    oscore_mutex_unlock(buffer_busy);
}

void printbuffer_draw_default(){
    printbuffer_draw(foxel35_bits, font_width_def, font_height_def, 4);
}