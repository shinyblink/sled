// Test Pattern
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

static int modno;
int init(int moduleno, char* argstr) {
    modno = moduleno;
	return 0;
}

#define TILESIZE 32

static void mset(int x, int y, RGB c){
    if ((x < 0) || (x >= matrix_getx()) || (y < 0) || (y >= matrix_gety())) return; 
    matrix_set(x,y,c);
}

static RGB colorwheel(int angle){
    angle = angle % 1536;
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

static RGB border = RGB(0, 0, 100);
static RGB origin = RGB(255,255,255);
static RGB text_counter = RGB(255, 255, 255);
static RGB text_coords = RGB(100, 255, 100);

static void printnum(int num, int x, int y, RGB color){
    // anchor is lower right corner
    int xx = x;
    if (num == 0){
        mset(xx-1,y, color);
        mset(xx,y-1, color);
        mset(xx-2,y-1, color);
        mset(xx-1,y-2, color);
    }
    while (num) {
        if (num & 1){
            mset(xx,y, color);
            mset(xx,y-1, color);
            mset(xx,y-2, color);
            xx -= 2;
        } else {
            mset(xx-1,y, color);
            mset(xx,y-1, color);
            mset(xx-2,y-1, color);
            mset(xx-1,y-2, color);
            xx -= 4;
        }
        num >>= 1;
    }
}

int draw(int argc, char* argv[]) {
    matrix_clear();
    int xsize = matrix_getx() / TILESIZE;
    int ysize = matrix_gety() / TILESIZE;
    int colorwheelcolor = 0;
    for (int yb = 0; yb < ysize; yb++){
        int yy = yb * TILESIZE + randn(TILESIZE-5) + 1;
        for (int x = 0; x < matrix_getx(); x++){
            mset(x,yy,colorwheel(colorwheelcolor));
            mset(x,yy+1,colorwheel(colorwheelcolor));
            mset(x,yy+2,colorwheel(colorwheelcolor));
        }
        colorwheelcolor += 200;
    }
    for (int xb = 0; xb < xsize; xb++){
        int xx = xb * TILESIZE + randn(TILESIZE-5) + 1;
        for (int y = 0; y < matrix_gety(); y++){
            mset(xx,y,colorwheel(colorwheelcolor));
            mset(xx+1,y,colorwheel(colorwheelcolor));
            mset(xx+2,y,colorwheel(colorwheelcolor));
        }
        colorwheelcolor += 200;
    }
    int counter = 0;
    for (int yb = 0; yb < ysize; yb++){
        for (int xb = 0; xb < xsize; xb++){
            int xo = xb*TILESIZE; //origin
            int yo = yb*TILESIZE; //origin
            for (int i = 0; i < TILESIZE; i++){
                mset(xo+i,yo,border); // upper
                mset(xo+i,yo+TILESIZE-1,border); //lower
                mset(xo,yo+i,border); //left
                mset(xo+TILESIZE-1,yo+i,border); //right
            }
            printnum(counter, xo + TILESIZE-2, yo+TILESIZE-2, text_counter);
            printnum(xb, xo + TILESIZE-6, yo+TILESIZE-6, text_coords);
            int xx = xo + TILESIZE -2;
            int yy = yo + TILESIZE -6;
            mset(xx,yy, text_coords);
            mset(xx-1,yy-1, text_coords);
            mset(xx-2,yy-2, text_coords);
            mset(xx,yy-2, text_coords);
            mset(xx-2,yy, text_coords);
            printnum(yb, xo + TILESIZE-6, yo+TILESIZE-10, text_coords);
            yy = yo + TILESIZE - 10;
            mset(xx-1,yy, text_coords);
            mset(xx-1,yy-1, text_coords);
            mset(xx-2,yy-2, text_coords);
            mset(xx,yy-2, text_coords);
            //matrix_set(x * TILESIZE + x, y * TILESIZE + y,col1);
            counter++;
        }
    }
   
    matrix_render();
	timer_add(udate()+5000000, modno, 0, NULL);
    return 0;
}

void reset(void) {
    matrix_clear();
	// Nothing?
}

void deinit(void) {}
