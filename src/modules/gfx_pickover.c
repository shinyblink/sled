// Pickover System
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
#include <stdio.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static unsigned int frame;
static oscore_time nexttick;

typedef vec3 Point;

// performance parameter
#define STABLE_DT 2.0
#define ITER_MAX 1000

// scoll max value
#define Z_MAX 5

// EQUATION PARAMETER
static float params[4];

// canvas size
static unsigned int xmax;
static unsigned int ymax;

// inverse of canvas size
static float inv_xmax;
static float inv_ymax;

// sliding param and increment
static float z;
static float inc_z;

// test point
static Point point;

// scaling factor
static const float xy_scale = 10;


int init(int moduleno, char* argstr) {

	modno = moduleno;
	frame = 0;

	// get canvas size
	xmax = matrix_getx();
	ymax = matrix_gety();
	inv_xmax = 1.0 / xmax;
	inv_ymax = 1.0 / ymax;

	// randomize starting scroll parameters z and inc_z
	z = (1 - 2.0*rand()/(float)(RAND_MAX)) * 1.0;
	inc_z = z > 0 ? -0.01 : 0.01;

	// initialize params
	params[0] = 1;
	params[1] = 1.8;
	params[2] = 0.71;
	params[3] = 1.51;

	// randomize params
	u_int8_t to_change = rand()%5;
	u_int8_t changed[4] = {0,0,0,0};
	u_int8_t c_count = 0;

	while(c_count < to_change) {
		u_int8_t c = rand()%4;
		if(changed[c] == 0) {
			c_count++;
			changed[c] = 1;
			params[c] += (1.0 - 2.0 * rand() / (float)(RAND_MAX)) * 0.2;
		}
	}

	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;

	// slightly adjust scroll speed
	inc_z += (1-2.0*rand()/(float)(RAND_MAX)) * 0.002;

	// radomize scroll direction
	inc_z = rand()%2 ? inc_z : -inc_z;

	// Prevent running to far from z=0
	if( z < 0 && z < -Z_MAX && inc_z < 0 ||
		z > 0 && z > Z_MAX && inc_z > 0 ) {
		inc_z = -inc_z;
	}
}

// numerical function iterator
static float pickover_int( Point* p ) {
	float dx = sin( params[0] * p->y ) - p->z * cos( params[1] * p->x );
	float dy = p->z * sin( params[4] * p->x ) - cos( params[3] * p->y );
	float dz = sin( p->x );

	float ret = (p->x - dx) * (p->x - dx) +
		        (p->y - dy) * (p->y - dy) +
		        (p->z - dz) * (p->z - dz);

	p->x = dx;
	p->y = dy;
	p->z = dz;

	return ret;
}

int draw(int _modno, int argc, char* argv[]) {

	// do shader stuff badly on a single core
	for (uint x = 0; x < xmax; x++ ) {
		for ( uint y = 0; y < ymax; y++ ) {

			point.x = x * inv_xmax * xy_scale;
			point.y = y * inv_ymax * xy_scale;
			point.z = z;

			for( int count=0; count < ITER_MAX; ++count ) {
				float dt = pickover_int(&point);
				if (dt < STABLE_DT) {
					break;
				}
			}

			float r = point.x < 0 ? -point.x : point.x;
			float g = point.y < 0 ? -point.y : point.y;
			float b = point.z < 0 ? -point.z : point.z;

			float dist = sqrt(r*r + g*g + b*b);

			r = r > 1 ? 255.0 * r / dist : 255.0;
			g = g > 1 ? 255.0 * g / dist : 255.0;
		    b = 255.0 * b/(b+1.0);

			RGB c = RGB(255 - (uint)r, 255 - (uint)g, (uint)b);
			matrix_set(x,y,c);
		}
	}

	z += inc_z;

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
