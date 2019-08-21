// Draws white dots that burn away
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

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS) * 10

static int modno;
static oscore_time nexttick;

static int mx,my;

static int * field;

static int rmax;
static int rmin;
static int threshold;

static char interesting = 0;

void reset(int _modno) {
    rmax = -1000;
    rmin = 1000;
    for (int * p=field; p<field+mx*my; p++) {
        int rr = rand();
        int r = 0;
        for (int i=0; i<8; i++) {
            r += (rr>>i*4)&0xf;
        }
        //r /= 2;
        rmax = (rmax<r)?r:rmax;
        rmin = (rmin>r)?r:rmin;
        *p = 1+r;
    }
    for (int * p=field; p<field+mx*my; p++) {
        *p -= (rmin);
    }
    threshold = (rmax-rmin);
    nexttick = udate();
    matrix_clear();
    interesting = 1;
}

static void calc() {
    interesting = 0;
    if (threshold) threshold--;

    for (int i=0; i<mx; i++) {
        for (int j=0; j<my; j++) {
            int * px = (field+i+j*mx);
            if (*px == 255) {
                *px = 0;
                interesting = 1;
            }
            else if (*px >= threshold) {

                *px *= 1.1;
                if (*px >= 256) {
                    *px = 255;
                }
                interesting = 1;
            }
            else if (*px > 0) {
                *px += 1;
                interesting = 1;
            }
        }
    }

}

int draw(int _modno, int argc, char* argv[]) {
    calc();
    for (int i=0; i<mx; i++) {
        for (int j=0; j<my; j++) {
            char px = *(field+i*my+j);
            matrix_set(i,j,RGB(px,px,px));
        }
    }

    matrix_render();
    if (interesting == 0) {
        return 1;
    }

    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    field = malloc(mx*my*sizeof(int));
    modno = moduleno;
    return 0;
}

void deinit(int _modno) {
    free(field);
}
