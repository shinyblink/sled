// Matrix-style runners moving from top to bottom.
// However, in random colors!
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
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
#include <stdio.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx;
static int my;

typedef struct runner {
	RGB color;
	int pos_x;
	int pos_y;
	int speed;
} runner;

static int numrunners;
static runner* runners;
static int HYPERSPEED = 3;


static void randomize_runner(runner * runner) {
    runner->color = HSV2RGB(HSV(randn(255), 255, 255));
    if (randn(4)==0){
        runner->pos_y = 0;
    } else if (runner->pos_y != my){
        runner->pos_y = randn(my-1);
    } else {
        runner->pos_y = randn(my/8);
    }
    runner->pos_x = randn(mx-1);
    runner->speed = randn(2)+1;
}

static RGB greenify(int intensity){
    if (intensity > 511) intensity = 511;
    if (intensity < 256) {
        return RGB(0,intensity,intensity/2);
        //return RGB(0,intensity/2,intensity);
    } else {
        return RGB(intensity-256,255,intensity/2);
        //return RGB(intensity-256,255,intensity/2);
    }
}

static int degreenify(RGB px){
    if (px.red){
        return px.red + 256;
    } else {
        return px.green;
    }
}

int out_of_bounds(int x, int y){
    return (x >= mx) || (x < 0) || (y >= my) || (y < 0);
}

void matrix_set_safe(int x, int y, RGB color){
    if (out_of_bounds(x,y)) return;
    matrix_set(x,y,color);
}

void run_runner(runner * runner){
    if (!randn(runner->speed)) runner->pos_y++;
    if (out_of_bounds(runner->pos_x, runner->pos_y)){
        randomize_runner(runner);
    } else {
        matrix_set(runner->pos_x, runner->pos_y, greenify(400));
    }
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numrunners = mx/2; // not sure if this is the best thing to do, but meh.
    printf("numrunners %d\n",numrunners);
	runners = malloc(numrunners * sizeof(runner));

    int runner;
    for (runner = 0; runner < numrunners; ++runner){
        randomize_runner(&(runners[runner]));
    }

	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	//matrix_clear();
	frame = 0;
}


int draw(int _modno, int argc, char* argv[]) {
    int x,y;
    for (x=0; x<mx; ++x){
        for (y=0;y<my; ++y){
            int intense_px = degreenify(matrix_get(x,y))*(256-HYPERSPEED*5)/256;
            RGB res = greenify(intense_px);
            matrix_set(x,y,res);
        }
    }
	int runner;
    int dont_care;
	for (runner = 0; runner < numrunners; ++runner) {
        for (dont_care=0;dont_care < HYPERSPEED; dont_care++){
            run_runner(&(runners[runner]));
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
	free(runners);
}
