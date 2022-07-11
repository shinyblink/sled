// Conway's Game of Life.
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
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <mathey.h>

#define FRAMETIME (T_SECOND / 60) // 4fps, sounds goodish.
#define FRAMES (TIME_MEDIUM * 60)

#define ALIVE 1
#define DEAD 0

#define POS(x, y) (y * matrix_getx() + x)

static int modno;
static int frame;
static oscore_time nexttick;

static RGB white = RGB(255, 255, 255);
static RGB black = RGB(0, 0, 0);
#define P_MAX 15
#define DELTA_T 0.001
typedef vec3 Point;
static Point p[P_MAX];

const float rho = 28.0;
const float sigma = 10.0;
const float beta = 8.0 / 3.0;

int init(int moduleno, char* argstr) {
	// doesn't look very great with anything less.
	if (matrix_getx() < 8)
		return 1;
	if (matrix_gety() < 8)
		return 1;

	modno = moduleno;
	frame = 0;

	for (int i = 0; i<P_MAX; i++)
	{
		p[i].x = random()/(float)(RAND_MAX)*10.;
		p[i].y = random()/(float)(RAND_MAX)*10.;
		p[i].z = random()/(float)(RAND_MAX)*10.;
	}

	matrix_clear();
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

	for (int x = 0; x < matrix_getx(); x++ )
	{
		for ( int y = 0; y < matrix_gety(); y++ )
		{
			RGB c = matrix_get(x,y);
			c.red *= 0.99;
			c.green *= 0.9;
			c.blue *= 0.8;
			matrix_set(x,y,c);
		}
	}


	for (int l = 0; l< 10; l++){
		for (int i =0; i<P_MAX; i++){
			lorenz_int(&p[i], DELTA_T);

			int x = p[i].x*4 + 128;
			int y = p[i].y*4 + 128;
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

void deinit(int _modno) {
}
