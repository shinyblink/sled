// Draws scrolling text.
//
// Copyright (c) 2019, Fridtjof Mund <fridtjof@das-labor.org>
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

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <random.h>
#include <assert.h>

#include "text.h"

#define TEXT_DEFAULT "RGB"
#define TEXT_DEFFRAMETIME (5000000 / matrix_getx())
// note that this rounds up in case of, say, 7
#define TEXT_MINH (((matrix_gety() + 1) / 2) - 4)
// "gap" of zeroes after text
#define TEXT_GAP matrix_getx()

static oscore_time text_nexttick, text_frametime;
static int text_position, text_moduleno;

static text* r_text = NULL;
static text* g_text = NULL;
static text* b_text = NULL;


int init(int moduleno, char* argstr) {
    text_position = 0;
    text_moduleno = moduleno;

    if (matrix_getx() < 8)
        return 1; // not enough X to be usable
    if (matrix_gety() < 7)
        return 1; // not enough Y to be usable

    return 0;
}

void reset(int _modno) {
    text_free(&r_text);
    text_free(&g_text);
    text_free(&b_text);

    text_position = 0;
}

void draw_letter(text * letter, int x_offset, int y_offset, int scale) {
    RGB bg_color = matrix_get(x_offset, y_offset);
    for (int y = y_offset; y < matrix_gety(); y++) {
        for (int x = x_offset; x < matrix_getx(); x++) {
            byte v = text_point(letter, (x - x_offset) / scale, (y - y_offset) / scale);

            // don't render black, we want to keep our background!
            if (v == 0)
                continue;

            RGB color = RGBlerp(v, bg_color, RGB(0xFF, 0xFF, 0xFF));
            matrix_set(x, y, color);
        }
    }
}

int draw(int _modno, int argc, char* argv[]) {
    if (text_position == 0) {
        r_text = text_render("R");
        if (!r_text)
            return 1;
        g_text = text_render("G");
        if (!g_text)
            return 1;
        b_text = text_render("B");
        if (!b_text)
            return 1;
        // Presumably this would be calculated based on an optional parameter or defaulting to TEXT_DEFFRAMETIME.
        text_nexttick = udate();
        text_frametime = TEXT_DEFFRAMETIME;
        // Add "center text & quit early" here
    } else if (text_position == (r_text->len + TEXT_GAP)) {
        text_position = 0;
        text_free(&r_text);
        text_free(&g_text);
        text_free(&b_text);
        return 1;
    }

    int x;
    int y;

    int x_third = matrix_getx() / 3;
    int x_max = matrix_getx();

    // draw full size RGB background bars
    for (y = 0; y < matrix_gety(); y++) {
        for (x = 0; x < matrix_getx(); x++) {
            RGB color;
            if (x <= x_third * 1) {
                color = RGB(0xFF, 0, 0);
            } else if (x <= x_third * 2) {
                color = RGB(0, 0xFF, 0);
            } else if (x <= x_max) {
                color = RGB(0, 0, 0xFF);
            } else {
                color = RGB(0xFF, 0xFF, 0xFF);
            }

            matrix_set(x, y, color);
        }
    }

    int x_sixth = matrix_getx() / 6;
    int y_offset = matrix_gety() / 2;

    int scale = 1;

    draw_letter(r_text, x_sixth * 1, y_offset, scale);
    draw_letter(g_text, x_sixth * 3, y_offset, scale);
    draw_letter(b_text, x_sixth * 5, y_offset, scale);

    matrix_render();
    text_position++;
    text_nexttick += text_frametime;
    timer_add(text_nexttick, text_moduleno, 0, NULL);
    return 0;
}

void deinit(int _modno) {
    // This acts conditionally on rendered being non-NULL.
    text_free(&r_text);
    text_free(&g_text);
    text_free(&b_text);
}

