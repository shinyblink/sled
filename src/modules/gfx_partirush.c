// Simple rainbow particles moving from the left side to the right.
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
static int colormode=0;

static int mx;
static int my;

typedef struct particle {
	RGB color;
	int pos_x;
	int pos_y;
	int speed;
} particle;

static int numparticles;
static particle* particles;

static RGB black = RGB(0, 0, 0);

static void randomize_particle(int particle) {

	particles[particle].pos_x = -randn(mx/4);
	particles[particle].pos_y = randn(my - 1);

    int randpool, randnumber;
    switch (colormode) {
        case 1:
            randpool = randn(1<<16);
            randnumber = 0;
            for (int i = 0; i < 4;i++){
                randnumber += (randpool >> (i*4)) & 0xf;
            }
            int randhue = (particles[particle].pos_y*256/my+randnumber-64)%256;
            particles[particle].color = HSV2RGB(HSV(randhue, 255-randnumber, (randpool & 0x7f)+0x80 ));
            break;
        case 0:
        default:
            particles[particle].color = RGB(randn(255), randn(255), randn(255));
            break;
    }


	int speed = 0;
	while (speed == 0)
		speed = randn(mx/8);
	particles[particle].speed = speed;
}

static void randomize_particles() {
	int particle;
	for (particle = 0; particle < numparticles; ++particle) {
		randomize_particle(particle);
	}
}

static void update_particles() {
	int particle;
	int x;
	for (particle = 0; particle < numparticles; ++particle) {
		x = particles[particle].pos_x + particles[particle].speed;

		if (x >= mx) {
			randomize_particle(particle);
		} else {
			particles[particle].pos_x = x;
		}
	}
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numparticles = (mx * my) / 8; // not sure if this is the best thing to do, but meh.
	particles = malloc(numparticles * sizeof(particle));


	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();

    colormode = (randn(10)==0)?1:0;

	randomize_particles();

	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	int particle;
	// clear out old particles
	for (particle = 0; particle < numparticles; ++particle)
		if (particles[particle].pos_x >= 0)
			matrix_set(particles[particle].pos_x, particles[particle].pos_y, black);

	// update the particles and draw them
	update_particles(); // todo, move back below matrix_render, to get a more consistant framerate
	for (particle = 0; particle < numparticles; ++particle)
		if (particles[particle].pos_x >= 0)
			matrix_set(particles[particle].pos_x, particles[particle].pos_y, particles[particle].color);

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
	free(particles);
}
