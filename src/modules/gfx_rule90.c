// Rule 90 Cellular Automata
// https://en.wikipedia.org/wiki/Rule_90#Rules
//
// Copyright (c) 2019, Matt Venn <matt@mattvenn.net>
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

#include <random.h>
#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stdlib.h>

#define FRAMETIME (T_SECOND / 8)
#define FRAMES (TIME_MEDIUM * 8)

#define ALIVE 1
#define DEAD 0

static int modno;
static RGB white = RGB(255, 255, 255);
static RGB black = RGB(0, 0, 0);
static int line;
static oscore_time nexttick;

// as rule90 only depends on the previous line, don't create a representation of the cell's status,
// use the matrix instead. Is this bad?
int init(int moduleno, char* argstr) {
    modno = moduleno;
    return 0;
}

static void rule90_init_line(int y) {
    int x;
    int cells = 0;
    // ensure there is at least one cell in the starting line
    while(cells == 0)
        for (x=0; x < matrix_getx(); ++x)
        {
            // try to scale the number of starting cells to the screen size
            int state = randn(matrix_getx() / 4) == 0;
            if (state) cells ++;
            matrix_set(x, y, state ? white : black);
        }
}

int rule90(int x, int y)
{
    int x_l = x - 1;
    int x_r = x + 1;

    if (x_l >= matrix_getx()) x_l -= matrix_getx();
    if (x_l < 0) x_l += matrix_getx();

    if (x_r >= matrix_getx()) x_r -= matrix_getx();
    if (x_r < 0) x_r += matrix_getx();

    RGB color_l = matrix_get(x_l,y);
    RGB color_r = matrix_get(x_r,y);

    return color_l.red ^ color_r.red;
}

int draw(int _modno, int argc, char* argv[]) {

    line ++;
    if(line > matrix_gety() -1)
        return 1;

    int cells = 0;

    for (int x=0; x < matrix_getx(); ++x)
    {
        int state = rule90(x, line-1);
        if (state) cells ++;
        matrix_set(x, line, state ? white : black);
    }

    if (cells == 0) rule90_init_line(line);

    matrix_render();

    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void reset(int _modno) {
    nexttick = udate();
    line = 0;
    matrix_clear();
    rule90_init_line(line);
}

void deinit(int _modno) {}
