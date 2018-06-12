// Simple checkerboard animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>

#define FPS 2
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS)

static int modno;
static int frame;
static ulong nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 2)
		return 1;
	if (matrix_gety() < 2)
		return 1;

	modno = moduleno;
	frame = 0;
	return 0;
}

static RGB white = RGB(255, 255, 255);
static RGB black = RGB(0, 0, 0);

void reset(void) {
	nexttick = udate();
	matrix_clear();
	frame = 0;
}


int draw(int argc, char* argv[]) {
	int mx = matrix_getx();
	int my = matrix_gety();

	int x;
	int y;
	for (y = 0; y < my; y++)
		for (x = 0; x < mx; x++)
			matrix_set(x, y, ((y + x + frame) % 2) ? black : white);

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
