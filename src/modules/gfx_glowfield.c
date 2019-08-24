// Please modify me as you please
// Additional variations are welcome
// have fun!
//
// Copyright (c) 2019, Jonathan Cyriax Brast
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
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <graphics.h>
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

#define NO_OF_DOTS 20

static int modno;
static int frame;
static oscore_time nexttick;

static int mx,my;
static int bx,by;

static float blur_factor;
static float blur_factor_dividend = 8; // blur_factor = min(mx,my)/blur_factor_dividend
static float max_intensity = 400.0f;
//static float max_max_intensity = 600.f;
//static float min_max_intensity = 200.f;

static float xs[NO_OF_DOTS];
static float ys[NO_OF_DOTS];
static float vs[NO_OF_DOTS];


static int blur_function(int radius_sq) {
    int intensity=0;
    if (radius_sq <= 0) intensity = max_intensity;
    else intensity = (int)max_intensity*1.0/((radius_sq+(blur_factor-1.0))/blur_factor);
    if (intensity > 511) intensity = 511;
    return intensity;
}

static RGB intense_red(int intensity){
    if (intensity > 511) intensity = 511;
    if (intensity < 256) {
        return RGB(0,intensity,intensity/2);
        //return RGB(0,intensity/2,intensity);
    } else {
        return RGB(intensity-256,255,intensity/2);
        //return RGB(intensity-256,255,intensity/2);
    }
}

static int intensity_from_pixel(RGB px){
    if (px.red){
        return px.red + 256;
    } else {
        return px.green;
    }
}


int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    blur_factor = ((mx<my)?mx:my)/blur_factor_dividend;
    modno = moduleno;
    frame = 0;
    return 0;
}


void reset(int _modno) {
    nexttick = udate();
    for (int i =0; i < NO_OF_DOTS; i++){
        xs[i] = (float)randn(mx-1);
        ys[i] = (float)randn(my-1);
        vs[i] = (float)(randn(50)+20)/100.0;
    }
    //vs[0] = 0.2;
    //vs[1] = 0.4;
    //vs[2] = 0.5;
    //vs[3] = 0.6;
    //vs[4] = 0.7;
    printf("%f %f %f %f %f\n", vs[0],vs[1],vs[2],vs[3],vs[4]);
    frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
    matrix_clear();
    int x_min = 0;
    int x_max = mx;
    int y_min = 0;
    int y_max = my;

    float phase = (frame%120)/120.0;

    for (int x = x_min; x<x_max; x++) {
        for (int y=y_min; y<y_max; y++) {
            int intense_px = intensity_from_pixel(matrix_get(x,y))*250/256;
            int intense_dot = 0;
            for (int i = 0;i<NO_OF_DOTS;i++){
                int bx = xs[i];
                int by = ys[i];
                int radius_sq = (x-bx)*(x-bx)+(y-by)*(y-by);
                float intensity = expf(-(phase-vs[i])*(phase-vs[i])*20.0);
                //printf("%f ",intensity);
                intense_dot += blur_function(radius_sq/9)*intensity/2;
            }
            RGB res = intense_red(intense_px+intense_dot);
            matrix_set(x,y,res);
            //matrix_set(x,y,RGB(255,255,255));
        }
    }
    //if (max_intensity > max_max_intensity) max_intensity = max_max_intensity;
    //if (max_intensity < min_max_intensity) max_intensity = min_max_intensity;
    //printf("--%d--\n",(int)max_intensity);



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
