// Simple projectile/ball animation.
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

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

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

static RGB colorwheel(int angle){
    angle = angle % 1536;
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

int init(int moduleno, char* argstr) {
	int mx = matrix_getx();
	int my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numballs = (mx * my) / 16; // not sure if this is the best thing to do, but meh.
    //numballs = 16;
	balls = malloc(numballs * sizeof(ball));

	modno = moduleno;
	frame = 0;
	return 0;
}

void randomize_balls(void) {
	int ball;
	int mx = matrix_getx();
	int my = matrix_gety();
    int color_offset = randn(1536);
	for (ball = 0; ball < numballs; ++ball) {
		//balls[ball].color = RGB(randn(255), randn(255), randn(255));
        balls[ball].color = colorwheel(randn(512)+color_offset);

		balls[ball].pos_x = randn(mx - 1);
		balls[ball].pos_y = randn(my - 1);

        do {
            balls[ball].vel_x = randn(8) - 4;
            balls[ball].vel_y = randn(8) - 4;
        } while (0 == (balls[ball].vel_x | balls[ball].vel_y));
        //printf("%i %i + %i %i\n",balls[ball].pos_x,balls[ball].pos_y,balls[ball].vel_x,balls[ball].vel_y);
	}
}

void update_balls(void) {
	int mx = matrix_getx();
	int my = matrix_gety();

	int ball_i;
	int x;
	int y;
	for (ball_i = 0; ball_i < numballs; ++ball_i) {
        ball * ball1 = balls +ball_i;
		x = ball1->pos_x;
		y = ball1->pos_y;
        if (ball1->vel_x && frame % ball1->vel_x == 0) {
            x += (ball1->vel_x > 0) ? 1 : -1;
            if (x < 0) {
                x = 0;
                ball1->vel_x = -ball1->vel_x;
            } else if (x >= mx) {
                x = mx - 1;
                ball1->vel_x = -ball1->vel_x;
            }
        }
        if (ball1->vel_y && frame % ball1->vel_y == 0) {
            y += (ball1->vel_y > 0) ? 1 : -1;
            if (y < 0) {
                y = 0;
                ball1->vel_y = -ball1->vel_y;
            } else if (y >= my) {
                y = my - 1;
                ball1->vel_y = -ball1->vel_y;
            }
        }
		ball1->pos_x = x;
		ball1->pos_y = y;
	}
    /*
    int ball1_i,ball2_i;
    for (ball1_i = 0; ball1_i < numballs; ++ball1_i){
        for (ball2_i = 0; ball2_i < ball1_i; ++ball2_i){
            ball *ball1 = &balls[ball1_i];
            ball *ball2 = &balls[ball2_i];
            if (ball1->pos_x != ball2->pos_x || ball1->pos_y != ball2->pos_y) continue;
            RGB tmpcolor = ball1->color;
            ball1->color = ball2->color;
            ball2->color = tmpcolor;
            //printf("foo");
        }
    }*/
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
