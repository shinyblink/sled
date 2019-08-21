// Derived from bttrblls
// Draws particles with peculiar behavior
//
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
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

#ifndef NO_TRAILS
// No trails is faster as only the last positions of each particle have to be cleared
#define NO_TRAILS false
#endif

static int modno;
static int frame;
static oscore_time nexttick;

typedef struct ball {
	RGB color;
	float pos_x;
	float pos_y;
	float vel_x;
	float vel_y;
    float acc_x;
    float acc_y;
} ball;

typedef struct well {
    float pos_x;
    float pos_y;
    float strength;
} well;

static int numballs;
static int numwells;
static ball* balls;
static well* wells;
static int mx;
static int my;

#if NO_TRAILS
static RGB black = RGB(0, 0, 0);
#endif

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
    }
    return RGB(0,0,0); // Should not happen
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numballs = (mx+my) ;
    //numwells = numballs / 10;
    numwells = 1;
    if (!numwells) numwells = 1;

    //numballs = 16;
	balls = malloc(numballs * sizeof(ball));
    wells = malloc(numwells * sizeof(well));

	modno = moduleno;
	frame = 0;
	return 0;
}

static void randomize(void) {
    int color_offset = randn(1536);
	for (int ball = 0; ball < numballs; ++ball) {
		//balls[ball].color = RGB(randn(255), randn(255), randn(255));
        balls[ball].color = colorwheel(randn(512)+color_offset);
        int x = randn(mx-1);
        int y = randn(my-1);
        int rx = +(y - my/2);
        int ry = -(x - mx/2);


		balls[ball].pos_x = (float)x;
		balls[ball].pos_y = (float)y;

        balls[ball].vel_x = ((float)randn(8))/10 * rx/mx;
        balls[ball].vel_y = ((float)randn(8))/10 * ry/my;
        //printf("%i %i + %i %i\n",balls[ball].pos_x,balls[ball].pos_y,balls[ball].vel_x,balls[ball].vel_y);
	}
    for (int well = 0; well < numwells; ++well){
        wells[well].pos_x = (float)randn((mx-1)/2)+mx/4+0.5;
        wells[well].pos_y = (float)randn((my-1)/2)+my/4+0.5;
        wells[well].strength = (float) (randn(5) + 1)*0.02;
    }
}

static void update_balls(void) {
	for (int ball_i = 0; ball_i < numballs; ++ball_i) {
        ball* b = balls + ball_i;
	    float x = b->pos_x;
		float y = b->pos_y;
	    float vx = b->vel_x;
		float vy = b->vel_y;
        float ax = 0;
        float ay = 0;
        for (int well_i = 0; well_i < numwells; ++well_i){
            well* w = wells + well_i;
            float dx = x - w->pos_x;
            float dy = y - w->pos_y;
            float r = hypotf(dx,dy);
            if (r < 3.0) continue;
            ax += - w->strength * dx / (r * r * r);
            ay += - w->strength * dy / (r * r * r);
        }
        for (int ball_j=0; ball_j < ball_i; ++ball_j){
            ball* b2 = balls + ball_j;
            float ddx = b2->pos_x -x;
            float ddy = b2->pos_y -y;
            float rr = hypotf(ddx,ddy);
            if (rr > 0.3) {
                float gx = ddx / (rr*rr*rr) * 0.01;
                float gy = ddy / (rr*rr*rr) * 0.01;
                b2->acc_x -= gx;
                b2->acc_y -= gy;
                ax += gx;
                ay += gy;
            }
            if (rr < 2.0) {
                float dvx = b2->vel_x - vx;
                float dvy = b2->vel_y - vy;
                dvx *= 0.02;
                dvy *= 0.02;
                b2->acc_x -= dvx;
                b2->acc_y -= dvy;
                ax += dvx;
                ay += dvy;
            }
            if (rr < 0.3) {
            }
            //b2->vel_x *= 0.98;
            //b2->vel_y *= 0.98;
            //vx *= 0.98;
            //vy *= 0.98;


        }
        b->acc_x = ax;
        b->acc_y = ay;

    }
	for (int ball_i = 0; ball_i < numballs; ++ball_i) {
        ball* b = balls + ball_i;
        float x = b->pos_x;
        float y = b->pos_y;
        float vx = b->vel_x;
        float vy = b->vel_y;
        float ax = b->acc_x;
        float ay = b->acc_y;
        vx += ax;
        vy += ay;
        x += vx;
        y += vy;
        if (x < 0){ x = 0; vx = -vx;}
        if (y < 0){ y = 0; vy = -vy;}
        if (x >= mx-1) { x = mx-1;  vx = -vx;}
        if (y >= my-1) { y = my-1;  vy = -vy;}
        b->pos_x = x;
        b->pos_y = y;
        b->vel_x = vx;
        b->vel_y = vy;
        b->acc_x = 0;
        b->acc_y = 0;
    }
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	randomize();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	int ball;
	// clear out old balls
#if NO_TRAILS
	for (ball = 0; ball < numballs; ++ball){
        int x = (int)balls[ball].pos_x;
        int y = (int)balls[ball].pos_y;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= mx) x = mx -1;
        if (y > my) y = my -1;
		matrix_set(x, y, black);
    }
#else
    for (int x = 0;x < matrix_getx();x++){
        for (int y = 0;y < matrix_gety();y++){
            RGB color;
            color = matrix_get(x,y);
            color.red = color.red * 230/256;
            color.green = color.green * 230/256;
            color.blue = color.blue * 230/256;
            matrix_set(x,y,color);
        }
    }
#endif

	// update the balls and draw them
	update_balls(); // todo, move back below matrix_render, to get a more consistant framerate
	for (ball = 0; ball < numballs; ++ball){
        int x = (int)balls[ball].pos_x;
        int y = (int)balls[ball].pos_y;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= mx) x = mx -1;
        if (y > my) y = my -1;
#define _min(a,b) a<b?a:b
        RGB this_color = balls[ball].color;
        RGB c = matrix_get(x,y);
        c.red = _min(c.red +this_color.red/5,255);
        c.green = _min(c.green + this_color.green/5,255);
        c.blue = _min(c.blue + this_color.blue/5,255);
        matrix_set(x,y,c);
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
	free(balls);
    free(wells);
}
