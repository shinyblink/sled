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
static float scale;
static oscore_time nexttick;


#define NUM_PARAMETERS 12
static float parameters[NUM_PARAMETERS];
static float goal_parameters[NUM_PARAMETERS];
static int mode;

static float randf() {
    return (1.0*randn(200000)-100000.0)/100000.0;
}


int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

    scale = 1./mx+1./my;
    scale *= 4;


	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();

    for (int i = 0; i < NUM_PARAMETERS; i++){
        parameters[i] = randf();
        goal_parameters[i] = randf();
    }
    mode = randn(1);

	frame = 0;
}


static float complex polynomial(float complex z){
    float complex pow = 1;
    float complex res1 = 0;
    float complex res2 = 0;
    switch (mode){

        case 1:
            for (int i = 0; i < NUM_PARAMETERS; i = i+2){
                res1 += pow * (parameters[i] + parameters[i+1]*I);
                pow *= z;
            }
            return res1;

        case 0:
        default:
            for (int i = 0; i < NUM_PARAMETERS/2; i=i+2){
                res1 += pow * (parameters[i] + parameters[i+1]*I -1 -I);
                res2 += pow * (parameters[i+NUM_PARAMETERS/2] + parameters[i+NUM_PARAMETERS/2+1]*I);
                pow *= z;
            }
            return res1/res2;
    }
}


static RGB color_function(float complex value){
            byte hue = (unsigned char)(255*(.5+carg(value)/(2*M_PI)));
            byte sat = (unsigned char)(255*(1-pow(.01,cabs(value))));
            float discont = log(cabs(value))-floor(log(cabs(value)));
            byte val = 96+(unsigned char)32*discont;
            return HSV2RGB(HSV(hue,sat,val));
}

int draw(int _modno, int argc, char* argv[]) {

    for (int i = 0; i < NUM_PARAMETERS; i++){
        parameters[i] = 0.99*parameters[i] + 0.01*goal_parameters[i];
    }

    if (frame % FPS == 0){
        for (int i = 0; i < NUM_PARAMETERS; i++){
            goal_parameters[i] = randf()*2;
        }
    }


    for (int x =0;x<mx;x++){
        for (int y=0;y<my;y++){
            float complex point = ((y-my/2)+(x-mx/2)*I)*scale;
            float complex value = polynomial(point);
            matrix_set(x, y, color_function(value));
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
}

#undef NUM_PARAMETERS
