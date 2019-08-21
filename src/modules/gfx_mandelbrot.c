// Simple mandelbrot zoomer.
// Far from perfect. But it works.
// Uses the escape time algorythm. Using HSV to color things.
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
#include <stddef.h>
#include <math.h>
#include <mathey.h>
#include <stdlib.h>
#include <stdio.h>
#include <taskpool.h>
#include <random.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS) * 3

#define ITERATIONS 255
#define MAXZOOM 10.0f

static int modno;
static int frame;
static oscore_time nexttick;

static int *iters;
static int mx;
static int my;

static volatile int min;
static volatile int max;
static oscore_mutex lock;

static byte color_offset = 0;

static float initial_size = 0.1;
static float initial_x = 0.314;
static float initial_y = 0.587;
static float end_size = 0.00362;
static float end_x = 0.35250;
static float end_y = 0.58206;

//static float initial_size = 2;
//static float initial_x = 0;
//static float initial_y = 0;
//static float end_size = 1;
//static float end_x = 0;
//static float end_y = 0;

#define PPOS(x, y) (x + (y * my))

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;

	iters = malloc((mx * my) * sizeof(int));
	if (iters == NULL)
		return 2;

	lock = oscore_mutex_new();

	modno = moduleno;
	frame = 0;
	return 0;
}

// simple scaling
static int rescale(int x, int maxn, int newn) {
	float f = (float) x / (float) maxn;
	return f * (float) newn;
}

#define SCALE(i, n) ((float) i / (float) n)
//#define FADE(start,end,part,total) (((float)start)+(SCALE(part,total)*((float)end-(float)start)))
static inline float FADE(float start, float end,int part, int total){
	return  start + SCALE(part,total)*(end-start);
}
static inline float LOG_FADE(float start, float end,int part,int total){
	return start * exp(log(end/start)*part/total);
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	frame = 0;
	min = 0;
	max = 0;
	color_offset = randn(255);
	switch (randn(7)){
		case 0:
			initial_size = 0.1;
			initial_x = 0.314;
			initial_y = 0.587;
			end_size = 0.00362;
			end_x = 0.35250;
			end_y = 0.58206;
			break;
		case 1:
			initial_size = 0.4;
			initial_x = -1.18;
			initial_y = 0;
			end_size = 0.07;
			end_x = -1.38;
			end_y = 0;
			break;
		case 2:
			initial_size = 0.058;
			initial_x = -1.3581;
			initial_y = -0.0654489;
			end_size = 0.0041473;
			end_x = -1.37270096;
			end_y = -0.08882466;
			break;
		case 3:
			initial_size = 0.01;
			initial_x = -1.256255;
			initial_y = -0.381321;
			end_size = 0.6;
			end_x = -1.2;
			end_y = -0.3;
			break;
		case 4:
			initial_size = 0.239;
			initial_x = 0.361;
			initial_y = -.056;
			end_size = 0.02;
			end_x = 0.395994;
			end_y = -0.1337;
			break;
		case 5:
			initial_size = 0.11;
			initial_x = -0.512;
			initial_y = 0.560;
			end_size = 0.003;
			end_x = -0.5490526;
			end_y = 0.651128;
			break;
		case 6:
			initial_size = 0.005;
			initial_x = -0.563659;
			initial_y = 0.6548416;
			end_size = 0.005;
			end_x = -0.5623978;
			end_y = 0.6426125;
			break;
        case 7:
			initial_size = 0.015;
			initial_x = -0.670225;
			initial_y = -0.458399;
			end_size = 0.0015;
			end_x = -0.6966326;
			end_y = -0.444189444277;
			break;


	}
}

void drawrow(void* row) {
	int py = *(int*) row;
	int px;

	if (py < 0 || py >= my) return;
	float size = LOG_FADE(initial_size,end_size,frame,FRAMES);
	float center_x = FADE(initial_x,end_x,frame,FRAMES);
	float center_y = FADE(initial_y,end_y,frame,FRAMES);
	float aspect_correction = SCALE(my,mx);
	float y0 = FADE(center_y-size/2.0*aspect_correction,center_y+size/2.0*aspect_correction,py,my);

	for (px = 0; px < mx; px++) {
		float x0 = FADE(center_x-size/2.0,center_x+size/2.0,px,mx);

		float x = 0;
		float y = 0;
		int i = 0;
		while ((x*x + y*y) <= (2*2) && i < ITERATIONS) {
			float xt = x*x - y*y + x0;
			y = 2 * x * y + y0;
			x = xt;
			i++;
			// we could lock this here. but performance.
			// data races are okay here.
			//oscore_mutex_lock(lock);
			if (i < min) min = i;
			if (i > max) max = i;
			//oscore_mutex_unlock(lock);
		}

		RGB col = RGB(0, 0, 0);
		if (i != (ITERATIONS)) {
			//oscore_mutex_lock(lock);
			if (min == max) max = min + 1;
			int mmin = min;
			int mmax = max;
			//oscore_mutex_unlock(lock);
			byte scaled = (rescale(i - mmin, mmax, 255)+color_offset)%256;;
			col = HSV2RGB(HSV(scaled, 255, 255));
		}
		matrix_set(px, py, col);
	}
}

int draw(int _modno, int argc, char* argv[]) {
	taskpool_forloop(TP_GLOBAL, &drawrow, 0, my);
	taskpool_wait(TP_GLOBAL);

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
	free(iters);
	oscore_mutex_free(lock);
}
