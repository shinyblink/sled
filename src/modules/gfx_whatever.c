// Whatever. I don't math.
//
// Copyright (c) 2021, Adrian "vifino" Pistol <vifino@tty.sh>
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
#include <graphics.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static int pos;
static uint frame = 0;
static oscore_time nexttick;

static int mat_x;
static int mat_y;
static double angle;

int init(int moduleno, char* argstr) {
	if ((mat_x = matrix_getx()) < 16)
		return 1;
	if ((mat_y = matrix_gety()) < 16)
		return 1;
	modno = moduleno;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

#define TAU (2 * M_PI)
#define TRI_RAD (TAU / 3)

typedef struct Point {
	int x;
	int y;
} Point;

static Point rotate_point(Point p, double radians, double radius) {
	p.x = p.x + (radius * sin(radians));
	p.y = p.y - (radius * cos(radians));
	return p;
}

static Point* polygon(Point p, double radians, double radius, uint n_edges) {
	Point* p_out = malloc(sizeof(Point) * n_edges);
	
	double incr = TAU/n_edges;

	for (int n = 0; n < n_edges; ++n) {
		p_out[n] = rotate_point(p, radians, radius);
		radians += incr;
	}
	return p_out;
}

static void draw_lines_between_points(Point* ps, uint count, RGB color) {
	for (int i = 0; i < count-1; ++i) {
		graphics_drawline(ps[i].x, ps[i].y, ps[i+1].x, ps[i+1].y, color);
	}
	graphics_drawline(ps[0].x, ps[0].y, ps[count-1].x, ps[count-1].y, color);
}

int draw(int _modno, int argc, char* argv[]) {
	matrix_clear();

	angle += 0.05;
	uint n_edges = 5; //(frame % 8);
	Point p = { mat_x/2, mat_y/2 };
	Point* ps = polygon(p, angle, MIN(mat_x, mat_y)/2, n_edges);
	draw_lines_between_points(ps, n_edges, RGB(255, 255, 255));
	free(ps);
	matrix_set(mat_x/2, mat_y/2, RGB(255, 0, 0));

	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		pos = 0;
		return 1;
	}
	frame++;
	pos++;;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
