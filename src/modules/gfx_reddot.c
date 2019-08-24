// Please modify me as you please
// Additional variations are welcome
// have fun!
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

#define FPS 20
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx,my;
static int bx,by;

static float blur_factor = 16.0f;
static int draw_radius = 10; // Doesn't matter right now
static float max_intensity = 400.0f;
static float max_max_intensity = 600.f;
static float min_max_intensity = 200.f;
static int frame_offset =0;
static float intensity_cycle_speed = 0.05;

static RGB blur_function(int radius_sq) {
    int intensity=0;
    if (radius_sq <= 0) intensity = max_intensity;
    else intensity = (int)max_intensity*1.0/((radius_sq+(blur_factor-1.0))/blur_factor);
    if (intensity > 511) intensity = 511;
    if (intensity < 256) {
        return RGB(intensity,0,0);
    } else {
        return RGB(255,intensity-256,intensity-256);
    }
}


int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    modno = moduleno;
    frame = 0;
    return 0;
}


void reset(int _modno) {
    nexttick = udate();
    matrix_clear();
    bx = randn(mx-1);
    by = randn(my-1);
    frame = 0;
    frame_offset = randn(6*(int)1.0/intensity_cycle_speed);
}

int draw(int _modno, int argc, char* argv[]) {
    //matrix_clear();
    int x_min = (bx-draw_radius>0)?bx-draw_radius:0;
    int x_max = (bx+draw_radius<mx)?bx+draw_radius:mx;
    int y_min = (by-draw_radius>0)?by-draw_radius:0;
    int y_max = (by+draw_radius<my)?by+draw_radius:my;
    x_min = 0;
    x_max = mx;
    y_min = 0;
    y_max = my;
    //printf("%d %d / %d %d\n",x_min,x_max,y_min,y_max);
    for (int x = x_min; x<x_max; x++) {
        for (int y=y_min; y<y_max; y++) {
            int radius_sq = (x-bx)*(x-bx)+(y-by)*(y-by);
            matrix_set(x,y,blur_function(radius_sq));
            //matrix_set(x,y,RGB(255,255,255));
        }
    }
    bx += randn(2)-1;
    by += randn(2)-1;
    max_intensity = min_max_intensity+(sin((frame_offset+frame)*intensity_cycle_speed)+1)*(max_max_intensity-min_max_intensity)+randn(100)-50;
 
    if (bx < 0) bx = 0;
    if (by < 0) by = 0;
    if (bx >= mx) bx = mx-1;
    if (by >= my) by = my-1;
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
