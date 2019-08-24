// Simple inverse FFT
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
#include <complex.h>

#define FPS 20
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static int frame;
static int mx, my;
static float sc;
static oscore_time nexttick;

typedef struct speclet {
    complex float pos;
    complex float val;
} speclet;
static float voffset, xoffset, yoffset,dxoffset,dyoffset;

static int numspeclets;
static speclet* spectrum;
static speclet* spectrum2;

static float randf() {
    return (1.0*randn(200000)-100000.0)/100000.0;
}


static RGB dim(RGB c,float val){
    return RGB(c.red*val,c.green*val,c.blue*val);
}
static RGB colorwheel(float fangle){
    fangle = fangle-floor(fangle);
    //printf("%f ", fangle);
    fangle *= 1536;
    int angle = fangle;
    //printf("%d ", angle);
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
    default: return RGB(0,0,0);
    }
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numspeclets = 10;
	spectrum = malloc(numspeclets * sizeof(speclet));
	spectrum2 = malloc(numspeclets * sizeof(speclet));

    sc = 1.0;

    //sc = 1.0/(mx>my?my:mx);
    //sc *= sc;

	modno = moduleno;
	frame = 0;
	return 0;
}

static void randomize(void) {
    sc = randf()*6;

    for (int i=0;i<numspeclets;i++){
        spectrum[i].pos = (randf()+randf()*I);
        spectrum[i].val = (randf()+randf()*I)/5;
        spectrum2[i].pos = (randf()+randf()*I);
        spectrum2[i].val = (randf()+randf()*I)/5;
	}
    xoffset=0;
    yoffset=0;
    dxoffset=randf()/10.0;
    dyoffset=randf()/10.0;
    voffset = randf();
}

static void update(void) {
    for (int i=0;i<numspeclets;i++){
        spectrum[i].val = 0.99*spectrum[i].val + 0.01*spectrum2[i].val;
        spectrum[i].pos = 0.99*spectrum[i].pos + 0.01*spectrum2[i].pos;
    }
    xoffset += dxoffset;
    yoffset += dyoffset;
    xoffset *= 0.95;
    yoffset *= 0.95;
    dxoffset += randf()/10.0;
    dyoffset += randf()/10.0;
    voffset += 0.01;
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	randomize();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
    update();
    for (int x =0;x<mx;x++){
        for (int y=0;y<my;y++){
            complex float pos =(x-mx/2.0)-xoffset - ((y-my/2.0)-yoffset)*I;
            pos *= sc * 0.05;
            complex float val = 0;
            for (speclet * s=spectrum;s<spectrum+numspeclets;s++){
                val += s->val*cexp(2*M_PI*I*creal(s->pos*pos));
            }
            //printf("%f%+fi  (%f %f)\n",creal(val),cimag(val), cabs(val), carg(val));
            matrix_set(x, y, colorwheel(cabs(val)+voffset));
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
	free(spectrum);
	free(spectrum2);
}
