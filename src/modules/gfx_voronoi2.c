// Voronoi Diagramm
//
// This effect is inspired by https://github.com/tsoding/
//
// Copyright (c) 2022, Olaf "Brolf" Pichler <brolf@magheute.net>
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
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <mathey.h>
#include <stdio.h>
#include <stdbool.h>
#include "taskpool.h"

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

static int modno;
static unsigned int frame;
static oscore_time nexttick;

static uint16_t xmax;
static uint16_t ymax;

static const double gamma_min = 1;
static const double gamma_max = 10;
static double gamma_exp; // will be reset
static double gamma_dt = 0.05;

static const float velocity = 0.4;
static const bool draw_points = false;

#define P_MAX 20
static struct Point {
    float x;
    float y;
    float vx;
    float vy;
} points[P_MAX];
static RGB points_color[P_MAX];

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
static const uint8_t palette_size = sizeof(palette)/sizeof(RGB);

static inline float dist(float x1, float y1, float x2, float y2)
{
    float dx = abs(x1 - x2);
    float dy = abs(y1 - y2);
    return powf(dx, gamma_exp) + powf(dy, gamma_exp);
}

static void myinit()
{
    assert(gamma_max > gamma_min);
    assert(gamma_min > 0.0f);

    float interval = gamma_max - gamma_min;
    gamma_exp = rand()/(float)(RAND_MAX) * interval + gamma_min;
    gamma_dt = rand()%2 ? gamma_dt : -gamma_dt;

    for (uint8_t i = 0; i < P_MAX; ++i) {
        points[i].x =  rand()/(float)(RAND_MAX)*(float)xmax;
        points[i].y =  rand()/(float)(RAND_MAX)*(float)ymax;

        points[i].vx = rand()/(float)(RAND_MAX)*velocity;
        points[i].vy = rand()/(float)(RAND_MAX)*velocity;

        if ( rand()%2 ) {
            points[i].vx = -points[i].vx;
        }
        if ( rand()%2 ) {
            points[i].vy = -points[i].vy;
        }
    }

    if(P_MAX < palette_size) {
        for(uint8_t i = 0; i < P_MAX; ++i) {
        choose_color:
            points_color[i] = palette[rand()%palette_size];

            for(uint8_t j = 0; j<i; ++j) {
                if(points_color[i].red   == points_color[j].red   &&
                   points_color[i].green == points_color[j].green &&
                   points_color[i].blue  == points_color[j].blue  &&
                   points_color[i].alpha == points_color[j].alpha) {
                    goto choose_color;
                }
            }
        }
    } else {
        uint8_t offset = rand()%palette_size;
        for(uint8_t i = 0; i < P_MAX; ++i) {
            points_color[i] = palette[(i+offset)%palette_size];
        }
    }
}

int init(int moduleno, char* argstr)
{
    xmax = matrix_getx();
    ymax = matrix_gety();

    myinit();

    modno = moduleno;
    frame = 0;

    return 0;
}

void reset(int _modno)
{
    myinit();

    nexttick = udate();
    frame = 0;
}

static void drawrow(void* y_ptr){
    uint16_t y = *((int*) y_ptr);

    for (uint16_t x = 0; x < xmax; ++x) {
        uint16_t j = 0;
        for (uint8_t i = 1; i < P_MAX; ++i) {
            if (dist(points[i].x, points[i].y, x, y) < dist(points[j].x, points[j].y, x, y)) {
                j = i;
            }
        }
        matrix_set(x,y,points_color[j]);
    }
}

int draw(int _modno, int argc, char* argv[])
{
    gamma_exp += gamma_dt;
    if (gamma_exp >= gamma_max || gamma_exp <= gamma_min )
        gamma_dt = -gamma_dt;

    taskpool_forloop(TP_GLOBAL, &drawrow, 0, ymax);
    taskpool_wait(TP_GLOBAL);

    for (uint8_t i = 0; i < P_MAX; ++i) {
        if (draw_points) {
            matrix_set((uint16_t)points[i].x, (uint16_t)points[i].y, black);
        }

        points[i].x += points[i].vx;
        points[i].y += points[i].vy;

        if(points[i].x <= 0) {
            points[i].x = 0.1;
            points[i].vx = -points[i].vx;
        } else if(points[i].x >= xmax) {
            points[i].x = xmax-0.1;
            points[i].vx = -points[i].vx;
        }

        if(points[i].y <= 0) {
            points[i].y = 0.1;
            points[i].vy = -points[i].vy;
        } else if(points[i].y >= ymax) {
            points[i].y = ymax-0.1;
            points[i].vy = -points[i].vy;
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
