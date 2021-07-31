// Please modify me as you please
// Additional variations are welcome
// have fun!
//
// Copyright (c) 2021, Jonathan Cyriax Brast
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

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx,my;
static int * field;
static int * water;


int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    // invisible 1px frame around the matrix
    field = malloc(sizeof(int)*(mx+2)*(my+2));
    water = malloc(sizeof(int)*(mx+2)*(my+2));
    modno = moduleno;
    frame = 0;
    return 0;
}

static RGB waterish(int v){
    return RGB(MIN(255,v/8),MIN(255,v/8),MIN(255,v/2));
}


#define FMAX 500
#define FLOWDIV 3
void reset(int _modno) {
    for (int i=0;i<(mx+2)*(my+2);i++){
        field[i] = randn(FMAX);
        water[i] = randn(FMAX/2);
    }
    for (int i=0;i<(mx+2)*(my+2)/4;i++){
        int r = randn(FMAX);
        for (int x=0; x<2;x++){
            for (int y=0; y<2;y++){
                field[2*i+x+(mx+2)*y] = r;
            }
        }
    }
    for (int i=0;i<(mx+2)*(my+2)/16;i++){
        int r = randn(FMAX);
        for (int x=0; x<4;x++){
            for (int y=0; y<4;y++){
                field[16*i+x+(mx+2)*y] = r;
            }
        }
    }
    nexttick = udate();
    frame = 0;
}

static int flow(int i, int j, int fallpressure){
    int resistance = MAX(field[j]-water[j],field[i]-water[i])/5;
    if (resistance<0) resistance = 0;
    int capacity = MIN(FMAX-water[j],water[i]);
    int diffpressure = water[i] - water[j];

    int flow = MIN(capacity,(diffpressure+fallpressure-resistance));
    if (flow < 0) flow = 0;
    water[i] -= flow;
    water[j] += flow;
    return flow;
}

static void calculate_flow(){
    for (int x = 0; x < mx+2; x++){ // source
        water[x] = (randn(FMAX)+water[x/2]+water[x/3])/3;
    }
    for (int x = 1; x<mx+1; x++) {
        for (int y=my; y>=0; y--) { // go from bottom so a droplet doesn't get pushed all the way down in one go
            int i = x + (mx+2) * y;
            int flown = 0;
            flown += flow(i,i+mx+2,10); // down
            flown += flow(i,i-1,0);     // left
            flown += flow(i,i+1,0);     // right
            flown += flow(i,i+mx+3,5);  // down right
            flown += flow(i,i+mx+1,5);  // down left
        }
    }
    for (int x = 0; x<mx+2; x++){ // drain
        water[(mx+2)*(my+1)+x] = 0;
    }
}


int draw(int _modno, int argc, char* argv[]) {
    calculate_flow();
    for (int x = 0; x<mx; x++) {
        for (int y=0; y<my; y++) {
            matrix_set(x,y,waterish(water[(x+1)+(mx+2)*(y+1)]));
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

void deinit(int _modno) {
    free(field);
    free(water);
}
