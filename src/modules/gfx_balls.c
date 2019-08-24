// Simple projectile/ball animation.
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

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

typedef struct ball {
	RGB color;
	int pos_x;
	int pos_y;
	int vel_x;
	int vel_y;
} ball;

static int numballs;
static ball* balls;

static RGB black = RGB(0, 0, 0);

int init(int moduleno, char* argstr) {
	int mx = matrix_getx();
	int my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numballs = (mx * my) / 16; // not sure if this is the best thing to do, but meh.
	balls = malloc(numballs * sizeof(ball));

	modno = moduleno;
	frame = 0;
	return 0;
}

static void randomize_balls(void) {
	int ball;
	int mx = matrix_getx();
	int my = matrix_gety();
	for (ball = 0; ball < numballs; ++ball) {
		balls[ball].color = RGB(randn(255), randn(255), randn(255));

		balls[ball].pos_x = randn(mx - 1);
		balls[ball].pos_y = randn(my - 1);

		balls[ball].vel_x = randn(8) - 4;
		balls[ball].vel_y = randn(8) - 4;
	}
}

static void update_balls(void) {
	int mx = matrix_getx();
	int my = matrix_gety();

	int ball;
	int x;
	int y;
	for (ball = 0; ball < numballs; ++ball) {
		x = balls[ball].pos_x + balls[ball].vel_x;
		y = balls[ball].pos_y + balls[ball].vel_y;

		if (x < 0) {
			x = 0;
			balls[ball].vel_x = -balls[ball].vel_x;
		}
		if (y < 0) {
			y = 0;
			balls[ball].vel_y = -balls[ball].vel_y;
		}

		if (x >= mx) {
			x = mx - 1;
			balls[ball].vel_x = -balls[ball].vel_x;
		}
		if (y >= my) {
			y = my - 1;
			balls[ball].vel_y = -balls[ball].vel_y;
		}

		balls[ball].pos_x = x;
		balls[ball].pos_y = y;
	}
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	randomize_balls();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	int ball;
	// clear out old balls
	for (ball = 0; ball < numballs; ++ball)
		matrix_set(balls[ball].pos_x, balls[ball].pos_y, black);

	// update the balls and draw them
	update_balls(); // todo, move back below matrix_render, to get a more consistant framerate
	for (ball = 0; ball < numballs; ++ball)
		matrix_set(balls[ball].pos_x, balls[ball].pos_y, balls[ball].color);

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
	free(balls);
}
