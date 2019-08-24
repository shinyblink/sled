// Sorting visualizations
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
#include <math.h>
#include <stdio.h>

#include "gfx_sort1D_algos.c"
// declares:
//      static int * data;
//      static int n;
//      static int sorting_algorithm;
//      static int h1;
//      static int h2;


// Speed settings
#define FPS 30
#ifndef GFX_SORT_1D_TIME
//#define GFX_SORT_1D_TIME 1
#endif
#define END_WAIT_TIME (T_SECOND /2)

#ifdef GFX_SORT_1D_TIME
    #define TARGET_FRAMES (GFX_SORT_1D_TIME * FPS)
    #define TARGET_TIME (GFX_SORT_1D_TIME * T_SECOND)
#else
    #define TARGET_FRAMES (TIME_MEDIUM * FPS)
    #define TARGET_TIME   (TIME_MEDIUM * T_SECOND)
#endif

#define RANGIFY(dir, var)\
   if (var < 0) var = 0;\
   if (var >= m##dir ) var = m##dir -1;


static int modno;
static int frame;
static oscore_time nexttick;
static oscore_time frame_time;
static int hyper_speed;
static int mx, my;

static int draw_style=0;
static int highlight_style=1;

static int sort_frames=0;

static RGB colorwheel(int angle){
    //angle = angle % 1536;
    int t = (angle / 256)%6;
    int v = angle % 256;
    switch (t){
    case 0: return RGB(255,v,0);
    case 1: return RGB(255-v,255,0);
    case 2: return RGB(0,255,v);
    case 3: return RGB(0,255-v,255);
    case 4: return RGB(v,0,255);
    case 5: return RGB(255,0,255-v);
    default: return RGB(0,0,0);
    }
}


void randomize_and_reset(){
    data[0] = 1;
    for (int i=1; i<mx; i++) {
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
    sorting_algorithm = randn(SORTING_ALGORITHM_MAX_ID);
    //sorting_algorithm = 14;
    draw_style = randn(2);
    //draw_style = 1;
    highlight_style = randn(2)+1;
    //highlight_style = 3;
    matrix_clear();
    frame = 0;
    sort_frames = 0;

    // predict time
    int predicted_steps = predict(sorting_algorithm);
    frame_time = TARGET_TIME/predicted_steps;
    hyper_speed = ceil(predicted_steps *1.0/ TARGET_FRAMES);
    frame_time *= hyper_speed;
    //printf("\n hyper %d, frame %d pred time %f\n",hyper_speed,frame_time,1.0*predicted_steps/hyper_speed*frame_time/T_SECOND);

}

static void draw_dots(){
    matrix_clear();
    int x1,x2,y1,y2;

    if (highlight_style & 1)
    for (int i = 0;i<2;i++){
        int hx;
        if (i) hx = h1; else hx = h2;
        if (hx < 0) continue;
        int hy = (data[hx])*my/n;
        if (hx <= 1) x1 = 0; else x1 = hx-1;
        if (hx >= mx-2) x2 = mx-1; else x2 = hx+1;
        if (hy >= my) y1 = 0; else y1 = my-hy-1;
        if (hy <= 1) y2 = my-1; else y2 = my-hy+1;
        for (int x=x1;x<=x2;x++){
            for (int y=y1;y<=y2;y++){
                matrix_set(x,y,RGB(255,255,255));
            }
        }
    }
    if (highlight_style & 2)
    if (h1 >= 0 && h2 >= 0){
        int x1=(h1<h2)?h1:h2;
        int x2=(h1<h2)?h2:h1;
        int y1 = my-(data[h1]*1.0)*my/n;
        int y2 = my-(data[h2]*1.0)*my/n;
        for (int x=x1;x<=x2;x++){
            matrix_set(x,y1,RGB(80,80,80));
            matrix_set(x,y2,RGB(80,80,80));
        }
    }
    for (int x=0; x<mx; x++) {
        int y = my-((data[x]*1.0)*my/n);
        RANGIFY(y,y);
        matrix_set(x,y,colorwheel(data[x]*1000/mx));
    }
}

static void draw_bars(){
    matrix_clear();
    if (h1 >= 0 || h2 >= 0) {
        for (int y=0; y<my; y++) {
            if (h1 >= 0) matrix_set(h1,y,RGB(80,80,80));
            if (h2 >= 0) matrix_set(h2,y,RGB(80,80,80));
        }
    }
    for (int x=0; x<mx; x++) {
        int height = ((data[x])*my/n);
        for (int y=my-height; y<my; y++) {
            RGB color = colorwheel(data[x]*1000/mx);
            matrix_set(x,y,color);
        }
    }
}

static void draw_lines_helper_move(){
    if (hyper_speed > my) return;
    if (sort_frames < my) return;
    for (int y = 0;y<my-hyper_speed;y++){
        for (int x = 0;x<mx;x++){
            matrix_set(x,y,matrix_get(x,y+hyper_speed));
        }
    }
}
static void draw_lines_helper_draw(int offset){
    if (!offset) draw_lines_helper_move();
    int y;
    if (sort_frames < my){
        y = sort_frames%my;
    } else {
        y = my-hyper_speed+offset;
    }
    if (y<0) return;

    for (int x=0; x<mx; x++) {
        matrix_set(x,y,colorwheel(data[x]*1000/mx));
    }
    if (h1 >= 0) {
        RGB c = matrix_get(h1,y);
        c.red /= 1.4; c.green /= 1.4; c.blue /=1.4;
        matrix_set(h1,y,c);
    }
    if (h2 >= 0) {
        RGB c = matrix_get(h2,y);
        c.red /= 1.4; c.green /= 1.4; c.blue /=1.4;
        matrix_set(h2,y,c);
    }
}



static void draw_select(){
    switch (draw_style){
        default:
        case 0: return draw_bars();
        case 1: return draw_dots();
        case 2: return draw_lines_helper_draw(hyper_speed-1);
    }
}


int draw(int _modno, int argc, char* argv[]) {

    oscore_time thistick = udate();

    if (__rval==2){
        randomize_and_reset(0);
        __rval=0;
        return 1;
    }
    for (int i = 0;i<hyper_speed;i++) {
        if (!__rval) {
            __rval=sort();
            sort_frames++;
        }
        if (draw_style == 2) draw_lines_helper_draw(i);
    }
    if (__rval){h1=-1;h2=-1;}

    draw_select();
    matrix_render();

    if (__rval == 1){
        frame++;
        nexttick = thistick + END_WAIT_TIME;
        timer_add(nexttick, modno, 0, NULL);
        #if 0
        int pred = predict(sorting_algorithm);
        printf("\n Algo %d ran %d should %d diff %d frac %f",
                sorting_algorithm,
                sort_frames,
                pred,
                pred - sort_frames,
                pred*1.0/sort_frames);
        #endif
        __rval = 2;
        return 0;
    }
    frame++;
    nexttick = thistick + frame_time;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void reset(int _modno) {
    randomize_and_reset(0);
    nexttick = udate();
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    n = mx;
    data = malloc(n * sizeof(int));
    data2 = malloc(n * sizeof(int));
    modno = moduleno;
    frame = 0;
    return 0;
}

void deinit(int _modno) {
    free(data);
    free(data2);
}
