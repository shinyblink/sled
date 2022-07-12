// Lorenz System
//
// Copyright (c) 2022, Olaf "Brolf" Pichler <brolf@magheute.net>
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
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <mathey.h>

#define FRAMETIME (T_SECOND / 60)
#define FRAMES (TIME_LONG * 60)

static int modno;
static int frame;
static oscore_time nexttick;

static RGB white = RGB(255, 255, 255);

#define P_MAX 15
#define DELTA_T 0.001
typedef vec3 Point;
static Point p[P_MAX];

static const float rho = 28.0;
static const float sigma = 10.0;
static const float beta = 8.0 / 3.0;


static unsigned int xmax;
static unsigned int ymax;

static const unsigned int frame_xmax = 32;
static const unsigned int frame_ymax = 32;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 64)	return 1;
	if (matrix_gety() < 64)	return 1;

	xmax = matrix_getx();
	ymax = matrix_gety();

	modno = moduleno;
	frame = 0;

	for (int i = 0; i<P_MAX; i++) {
		p[i].x = random()/(float)(RAND_MAX)*10.;
		p[i].y = random()/(float)(RAND_MAX)*10.;
		p[i].z = random()/(float)(RAND_MAX)*10.;
	}

	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

static void lorenz_int( Point* p, float delta_t) {
	float dx = sigma * (p->y - p->x);
	float dy = p->x * (rho - p->z) - p->y;
	float dz = p->x* p->y - beta * p->z;

	p->x = p->x + delta_t * dx;
	p->y = p->y + delta_t * dy;
	p->z = p->z + delta_t * dz;
}

int draw(int _modno, int argc, char* argv[]) {
	for (int x = 0; x < matrix_getx(); x++ ) {
		for ( int y = 0; y < matrix_gety(); y++ ) {
			RGB c = matrix_get(x,y);
			c.red *= 0.99;
			c.green *= 0.9;
			c.blue *= 0.8;
			matrix_set(x,y,c);
		}
	}


	for (int l = 0; l < 10; l++) {
		for (int i = 0; i < P_MAX; i++) {
			lorenz_int(&p[i], DELTA_T);

			int x = p[i].x * (xmax/frame_xmax/2) + xmax/2;
			int y = p[i].y * (ymax/frame_ymax/2) + ymax/2;
			matrix_set(x, y, white);
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

void deinit(int _modno) {}
