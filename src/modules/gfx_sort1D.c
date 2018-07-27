// Sorting visualizations

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
#define FPS 60
#ifndef GFX_SORT_1D_TIME
//#define GFX_SORT_1D_TIME 30
#endif

#ifdef GFX_SORT_1D_TIME
    #define TARGET_FRAMES (GFX_SORT_1D_TIME * FPS)
    #define TARGET_TIME (GFX_SORT_1D_TIME * T_SECOND)
#else
    #define TARGET_FRAMES (RANDOM_TIME * FPS)
    #define TARGET_TIME   (RANDOM_TIME * T_SECOND)
#endif

static int modno;
static ulong frame;
static ulong nexttick;
static ulong frame_time;
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
    //sorting_algorithm = 1;
    draw_style = randn(2);
    highlight_style = randn(2)+1;
    matrix_clear();
    #if 0
    int expected_runtime = 0;
    while (sort() == 0) expected_runtime++;
    p rintf("\nRuntime Algo %d: %d frames\n",sorting_algorithm,expected_runtime);
    data[0] = 1;
    for (int i=1; i<mx; i++) {
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
    #endif
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
        int hy = (data[hx]-1)*my/n;
        if (hx <= 1) x1 = 0; else x1 = hx-1;
        if (hx >= mx-2) x2 = mx-1; else x2 = hx+1;
        if (hy <= 1) y1 = 0; else y1 = hy-1;
        if (hy >= my-2) y2 = mx-1; else y2 = hy+1;
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
        int y1 = (data[h1]-1)*my/n;
        int y2 = (data[h2]-1)*my/n;
        for (int x=x1;x<=x2;x++){
            matrix_set(x,y1,RGB(80,80,80));
            matrix_set(x,y2,RGB(80,80,80));
        }
    }
    for (int x=0; x<mx; x++) {
        int y = (data[x]-1)*my/mx;
        if (y < 0) y=0;
        if (y >= my) y=my-1;
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
        int range = (data[x])*my/mx;
        if (range >= my) range = my;
        if (range < 1) range = 1;
        for (int y=my-1; y>range-2; y--) {
            RGB color = colorwheel(data[x]*1000/mx);
            matrix_set(x,y,color);
        }
    }
}

static void draw_lines(){
    int y;
    if (sort_frames < my){
        y = sort_frames%my;
    } else {
        for (int y = 0;y<my-1;y++){
            for (int x = 0;x<mx;x++){
                matrix_set(x,y,matrix_get(x,y+1));
            }
        }
        y = my-1;
    }
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
        case 2: return draw_lines();
    }
}


int draw(int argc, char* argv[]) {

    ulong thistick = udate();

    if (__rval==1){
        randomize_and_reset();
        __rval=0;
    }
    for (int i = 0;i<hyper_speed;i++) {
        if (__rval=sort()) break;
        if (draw_style == 2) draw_select();
        sort_frames++;
    }

    draw_select();
    matrix_render();

    if (__rval > 0) {
        //printf("\nRan for %d frames\n",frame);
        #if 0
        int pred = predict(sorting_algorithm);
        printf("\n Algo %d ran %d should %d diff %d frac %f",
                sorting_algorithm,
                sort_frames,
                pred,
                pred - sort_frames,
                pred*1.0/sort_frames);
        #endif
        return 1;
    }

    frame++;
    nexttick = thistick + frame_time;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void reset(void) {
    randomize_and_reset();
    nexttick = udate();
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    n = mx;
    data = malloc(mx * sizeof(int));
    modno = moduleno;
    frame = 0;
    return 0;
}

int deinit() {
    free(data);
    return 0;
}
