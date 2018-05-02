// Simple rainbow particles moving from the left side to the right.

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

int mx;
int my;

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
	particles[particle].color = RGB(randn(255), randn(255), randn(255));

	particles[particle].pos_x = -randn(mx/4);
	particles[particle].pos_y = randn(my - 1);

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

	randomize_particles();

	modno = moduleno;
	frame = 0;
	return 0;
}

void reset() {
	frame = 0;
}

int draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = udate();
		matrix_clear();
	}

	int particle;
	// clear out old particles
	for (particle = 0; particle < numparticles; ++particle)
		if (particles[particle].pos_x >= 0)
			matrix_set(particles[particle].pos_x, particles[particle].pos_y, &black);

	// update the particles and draw them
	update_particles(); // todo, move back below matrix_render, to get a more consistant framerate
	for (particle = 0; particle < numparticles; ++particle)
		if (particles[particle].pos_x >= 0)
			matrix_set(particles[particle].pos_x, particles[particle].pos_y, &particles[particle].color);

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
	free(particles);
	return 0;
}
