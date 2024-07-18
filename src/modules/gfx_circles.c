// Circles
//
// How to draw circles without ever using a multiplication?
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
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static unsigned int frame;
static oscore_time nexttick;

static u_int16_t xmax;
static u_int16_t ymax;

// random seeds for color selection
static u_int16_t rs1;
static u_int16_t rs2;

typedef struct {
    u_int16_t x;   // x coordinate of center
    u_int16_t y;   // y coordinate of center
    u_int16_t r;   // r curent radius of circle
    u_int16_t lt;  // livetime (rmax) after which the circle should die
} Circle;

// Number of circles to draw
#define P_MAX 25
static Circle circles[P_MAX];

// Color palette and stuff
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
static const u_int16_t palette_size = sizeof(palette) / sizeof(RGB);


// xor the color value at the given position with `color`
static void matrix_xor( u_int16_t x, u_int16_t y, RGB color ){

    RGB tmp = matrix_get(x, y);
    tmp.red = tmp.red ^ color.red;
    tmp.green = tmp.green ^ color.green;
    tmp.blue = tmp.blue ^ color.blue;
    tmp.alpha = color.alpha;

    matrix_set(x, y, tmp);
}

static void check_or_reinit(Circle *c);
// reinitializes a circle given by the pointer `c`
static void reinit_circle( Circle *c ){

    c->x = rand()%(xmax);
    c->y = rand()%(ymax);
    c->lt = rand() % (( xmax > ymax ? xmax : ymax ) / 4) + 2;
    c->r = 1;
    // check if the coosen circle is valid
    check_or_reinit(c);
}

// checks if th circle given by pointer `c` fits inside matrix else it calls `reinit_circle`
static void check_or_reinit(Circle *c){

    if( !(xmax > c->x + c->r) || 
        !(c->x >= c->r) ||
        !(ymax > c->y + c->r) ||
        !(c->y >= c->r) ||
        !(c->lt > c->r)
    )
    {
        reinit_circle(c);
    }
}


// draws the circle `c` by using xor and a given `color_id`
// `color_id` maps to the `palette`
static void xor_circle(Circle *c, u_int16_t color_id){

    // randomly but consistently choose a color
    RGB color = palette[(color_id * rs1 + rs2) % palette_size];

    // asserts so we do not over or underflow
    assert(xmax > c->x + c->r);
    assert(c->x >= c->r);
    assert(ymax > c->y + c->r);
    assert(c->y >= c->r);

    // here comes the magic
    const u_int16_t cr2 = c->r+c->r;

    int16_t x = c->r;
    int16_t y = 0;
    int16_t dy = -2;
    int16_t dx = cr2+cr2 - 4;
    int16_t d = cr2 - 1;

    while(y <= x){
        matrix_xor( (c->y - y), (c->x - x), color);
        matrix_xor( (c->y - y), (c->x + x), color);
        matrix_xor( (c->y + y), (c->x - x), color);
        matrix_xor( (c->y + y), (c->x + x), color);

        matrix_xor( (c->y - x), (c->x - y), color);
        matrix_xor( (c->y - x), (c->x + y), color);
        matrix_xor( (c->y + x), (c->x - y), color);
        matrix_xor( (c->y + x), (c->x + y), color);

        d += dy;
        dy -= 4;
        ++y;

#if 0
        if(d < 0){
            d += dx;
            dx -= 4;
            --x;
        }
#else // why not make it branchless
        int Mask = (d >> 31);
        d += dx & Mask;
        dx -= -4 & Mask;
        x += Mask;
#endif
    }
}


// effect reset and initialisation
static void my_init(){

    rs1 = rand() % 65536;
    rs2 = rand() % 65536;

    // sometimes draw the circles on top of old effect
    if( rand() % 3 ) matrix_clear();
    else {
        static RGB tmp;
        for(u_int16_t x = 0; x < xmax; x++){
            for(u_int16_t y = 0; y < ymax; y++){
                tmp = matrix_get(x, y);
                matrix_set(x, y, RGB(tmp.red/2, tmp.green/2, tmp.blue/2));
            }
        }
    }

    for( u_int16_t i = 0; i < P_MAX; i++ ){
        // initialize the circles
        reinit_circle(&circles[i]);
        // darw the first circles
        xor_circle(&circles[i], i);
    }
}

int init(int moduleno, char* argstr){

    xmax = matrix_getx();
    ymax = matrix_gety();

    my_init();

    modno = moduleno;
    frame = 0;

    return 0;
}

void reset(int _modno) {

    my_init();

    nexttick = udate();
    frame = 0;
}

int draw(int _modno, int argc, char* argv[]){

    // slow the effect down a bit
    if (frame % 2 == 0){

        for (u_int16_t i = 0; i < P_MAX; ++i){
            // delete old trace
            xor_circle(&circles[i], i);
            // increase the radius of current circle
            circles[i].r += 1;
            // check if circle still valid
            check_or_reinit(&circles[i]);
            // paint new circle
            xor_circle(&circles[i], i);
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
