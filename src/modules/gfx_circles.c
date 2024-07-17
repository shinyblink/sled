// Circles
//
// Copyright (c) 2024, Olaf "brolf" Pichler <brolf@magheute.net>
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

#include <sys/types.h>
#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <mathey.h>
#include <stdio.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

static int modno;
static unsigned int frame;
static oscore_time nexttick;

static u_int16_t xmax;
static u_int16_t ymax;

#define P_MAX 25
typedef struct {
    u_int16_t x;
    u_int16_t y;
    u_int16_t r;
} Circle;
static Circle points[P_MAX];

static const RGB black = RGB(0,0,0);

static const RGB palette[] = {
    RGB(255, 0, 0),    RGB(204, 0, 0),    RGB(153, 0, 0),    RGB(255, 85, 85),
    RGB(204, 68, 68),  RGB(153, 51, 51),  RGB(255, 127, 0),  RGB(204, 102, 0),
    RGB(153, 76, 0),   RGB(255, 170, 85), RGB(204, 136, 68), RGB(153, 102, 51),
    RGB(255, 255, 0),  RGB(204, 204, 0),  RGB(153, 153, 0),  RGB(255, 255, 85),
    RGB(204, 204, 68), RGB(153, 153, 51), RGB(127, 255, 0),  RGB(102, 204, 0),
    RGB(76, 153, 0),   RGB(170, 255, 85), RGB(136, 204, 68), RGB(102, 153, 51),
    RGB(0, 255, 0),    RGB(0, 204, 0),    RGB(0, 153, 0),    RGB(85, 255, 85),
    RGB(68, 204, 68),  RGB(51, 153, 51),  RGB(0, 255, 127),  RGB(0, 204, 102),
    RGB(0, 153, 76),   RGB(85, 255, 170), RGB(68, 204, 136), RGB(51, 153, 102),
    RGB(0, 255, 255),  RGB(0, 204, 204),  RGB(0, 153, 153),  RGB(85, 255, 255),
    RGB(68, 204, 204), RGB(51, 153, 153), RGB(0, 127, 255),  RGB(0, 102, 204),
    RGB(0, 76, 153),   RGB(85, 170, 255), RGB(68, 136, 204), RGB(51, 102, 153),
    RGB(0, 0, 255),    RGB(0, 0, 204),    RGB(0, 0, 153),    RGB(85, 85, 255),
    RGB(68, 68, 204),  RGB(51, 51, 153),  RGB(127, 0, 255),  RGB(102, 0, 204),
    RGB(76, 0, 153),   RGB(170, 85, 255), RGB(136, 68, 204), RGB(102, 51, 153),
    RGB(255, 0, 255),  RGB(204, 0, 204),  RGB(153, 0, 153),  RGB(255, 85, 255),
    RGB(204, 68, 204), RGB(153, 51, 153), RGB(255, 0, 127),  RGB(204, 0, 102),
    RGB(153, 0, 76),   RGB(255, 85, 170), RGB(204, 68, 136), RGB(153, 51, 102)
};
static const u_int16_t palette_size = sizeof(palette)/sizeof(RGB);


static void matrix_xor( u_int16_t x, u_int16_t y, RGB color ){
    RGB tmp = matrix_get(x, y);
    tmp.red = tmp.red ^ color.red;
    tmp.green = tmp.green ^ color.green;
    tmp.blue = tmp.blue ^ color.blue;
    tmp.alpha = color.alpha;

    matrix_set(x, y, tmp);


//    printf("%i, %i, %i, %i\n", tmp.red, tmp.green, tmp.blue, tmp.alpha);

}

static void reinit_circle(Circle *c){
    c->x = rand()%(xmax/2) + xmax/4;
    c->y = rand()%(ymax/2) + ymax/4;
    c->r = 1;
}

static void check_or_reinit(Circle *c){
    if( !(xmax > c->x + c->r) || 
        !(c->x >= c->r) ||
        !(ymax > c->y + c->r) ||
        !(c->y >= c->r)
    )
    {
        reinit_circle(c);
        check_or_reinit(c);
    }
}

static void xor_circle(Circle *c, u_int16_t color_id) {

    assert(xmax > c->x + c->r);
    assert(c->x >= c->r);

    assert(ymax > c->y + c->r);
    assert(c->y >= c->r);

    const u_int16_t cr2 = c->r+c->r;

    int16_t x = c->r;
    int16_t y = 0;
    int16_t dy = -2;
    int16_t dx = cr2+cr2 - 4;
    int16_t d = cr2 - 1;


    while(y <= x)
    {
        matrix_xor( (c->y - y), (c->x- x), palette[color_id%palette_size]);
        matrix_xor( (c->y - y), (c->x + x), palette[color_id%palette_size]);
        matrix_xor( (c->y + y), (c->x - x), palette[color_id%palette_size]);
        matrix_xor( (c->y + y), (c->x + x), palette[color_id%palette_size]);

        matrix_xor( (c->y - x), (c->x - y), palette[color_id%palette_size]);
        matrix_xor( (c->y - x), (c->x + y), palette[color_id%palette_size]);
        matrix_xor( (c->y + x), (c->x - y), palette[color_id%palette_size]);
        matrix_xor( (c->y + x), (c->x + y), palette[color_id%palette_size]);

        d += dy;
        dy -= 4;
        ++y;

#if 0
        if(d < 0){
            d += dx;
            dx -= 4;
            --x;
        }
#else
        int Mask = (d >> 31);
        d += dx & Mask;
        dx -= -4 & Mask;
        x += Mask;
#endif
    }
}

int init(int moduleno, char* argstr)
{
    xmax = matrix_getx();
    ymax = matrix_gety();

    for( u_int16_t x = 0; x < xmax; x++){

        for( u_int16_t y = 0; y < ymax; y++){
            matrix_set(x,y, black);
        }
    }

    for( u_int16_t i = 0; i < P_MAX; i++ ){
        reinit_circle(&points[i]);
        xor_circle(&points[i], i);
    }

    modno = moduleno;
    frame = 0;

    return 0;
}

void reset(int _modno)
{

    for( u_int16_t x = 0; x < xmax; x++){

        for( u_int16_t y = 0; y < ymax; y++){
            matrix_set(x,y, black);
        }
    }

    for( u_int16_t i = 0; i < P_MAX; i++ ){
        reinit_circle(&points[i]);
        xor_circle(&points[i], i);
    }


    nexttick = udate();
    frame = 0;
}

int draw(int _modno, int argc, char* argv[])
{

    if (frame % 2 == 0){

        for (u_int16_t i = 0; i < P_MAX; ++i) {
            xor_circle(&points[i], i);
            points[i].r += 1;
            check_or_reinit(&points[i]);
            xor_circle(&points[i], i);
        }
    }

    matrix_render();

    if (frame >= FRAMES) {
        frame = 0;
        return 1;
    }
    frame++;
    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void deinit(int _modno) {}
