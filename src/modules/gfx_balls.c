// Simple projectile/ball animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS)

static int modno;
static int frame;
static ulong nexttick;

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

void randomize_balls() {
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

void update_balls() {
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
			balls[ball].vel_x = 1;
		}
		if (y < 0) {
			y = 0;
			balls[ball].vel_y = 1;
		}

		if (x >= mx) {
			x = mx - 1;
			balls[ball].vel_x = -1;
		}
		if (y >= my) {
			y = my - 1;
			balls[ball].vel_y = -1;
		}

		balls[ball].pos_x = x;
		balls[ball].pos_y = y;
	}
}

void reset(void) {
	nexttick = udate();
	matrix_clear();
	randomize_balls();
	frame = 0;
}

int draw(int argc, char* argv[]) {
	int ball;
	// clear out old balls
	for (ball = 0; ball < numballs; ++ball)
		matrix_set(balls[ball].pos_x, balls[ball].pos_y, &black);

	// update the balls and draw them
	update_balls(); // todo, move back below matrix_render, to get a more consistant framerate
	for (ball = 0; ball < numballs; ++ball)
		matrix_set(balls[ball].pos_x, balls[ball].pos_y, &balls[ball].color);

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

int deinit() {
	free(balls);
	return 0;
}
