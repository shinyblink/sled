// Some sort of noise function.
// Copyright (c) 2020, Jonathan Cyriax Brast
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
#include <timers.h>
#include <stdio.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

// calculate and print timing information
#define SORT_TIMING
// Use bitmask to store where updates happened
// Seems faster without on an i5
static int modno;
static int frame;
static oscore_time nexttick;

static oscore_time timer_n;

static int * data;
static int noise_size;
static int bias_size;
static int bias_offset;
static int bias_both_directions;

// SETTINGS
static const int s_generator_step = 8;

// GENERATED SETTINGS
static int color_range = 700;


static void own_reset();

static RGB colorwheel(int angle){
	//angle = angle % 1536;
    while (angle < 0) angle += 1536;
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

static void fill_data(){
	int mx = matrix_getx();
	int my = matrix_gety();
	int color_offset = randn(1536);
	for (int i=0;i<mx;i++){
		for (int j=0;j<my;j++){
			data[i+j*mx] = randn(color_range) + color_offset;
            matrix_set(i,j,colorwheel(data[i+mx*j]));
		}
	}
}

static void random_settings(){
noise_size=randn(50);
bias_size=randn(80);
bias_offset=randn(4)-2;
bias_both_directions=randn(0);

}

static void noisify(int *a, int* b, int* c, int* d){
    int p1,p2,avg,bias=0;;
    avg = (*a+*b+*c+*d)/4;
    bias = randn(bias_size)-randn(bias_size)+bias_offset;
    p1 = *a>avg?*a:avg;
    p2 = *a<avg?*a:avg;
    p1 += noise_size;
    p2 -= noise_size;
    *a = randn(p1-p2)+p2+bias;
    p1 = *b>avg?*b:avg;
    p2 = *b<avg?*b:avg;
    p1 += noise_size;
    p2 -= noise_size;
    *b = randn(p1-p2)+p2+bias;
    p1 = *c>avg?*c:avg;
    p2 = *c<avg?*c:avg;
    p1 += noise_size;
    p2 -= noise_size;
    *c = randn(p1-p2)+p2+bias;
    p1 = *d>avg?*d:avg;
    p2 = *d<avg?*d:avg;
    p1 += noise_size;
    p2 -= noise_size;
    *d = randn(p1-p2)+p2+bias;
}

static void noise(){
	int mx = matrix_getx();
	int my = matrix_gety();
    int size = mx*my;
    //printf("mx x my = %d x %d\n",mx,my);
    uint entropy = rand();

    // generate random points
    for (int i = 0;i<size;){
            if (entropy == 0) entropy = rand();
            int step = entropy % s_generator_step;
            entropy /= s_generator_step;
            i += step;
		    int * a = data+(i%size);
		    int * b = data+((i+1)%size);
		    int * c = data+((i+mx)%size);
		    int * d = data+((i+mx+1)%size);
		    noisify(a,b,c,d);

    }

}


int init(int moduleno, char* argstr) {
	int mx = matrix_getx();
	int my = matrix_gety();
	data = malloc(sizeof(int) * mx * my);
	modno = moduleno;
	frame = 0;
    timer_n = 0;
	return 0;
}

static void own_reset(){
	fill_data();

    // timing
}


void reset(int _modno) {
	own_reset();
	nexttick = udate();
	matrix_clear();
}

int draw(int _modno, int argc, char* argv[]) {
	int mx = matrix_getx();
	int my = matrix_gety();

    noise();
	for (int i=0;i<mx;i++){
		for (int j=0;j<my;j++){
                matrix_set(i,j,colorwheel(data[i+mx*j]));
		}
	}


	matrix_render();

    if (frame >= FRAMES) {
        frame = 0;
        return 1;
    }

    if (frame % (FPS*5)==0){
        random_settings();
    }

	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(data);
}
