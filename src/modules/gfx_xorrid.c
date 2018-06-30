// Weird XOR thing.

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

static int mx, my;

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(void) {
	nexttick = udate();
	frame = 0;
}

int draw(int argc, char* argv[]) {
	for (int y = 0; y < my; y++)
		for (int x = 0; x < mx; x++) {
			int b = (x * y) ^ (x * y + frame);
			matrix_set(x, y, RGB(0, 0, b));
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

int deinit() {
	return 0;
}
