// Rather simple rectangle animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>

#define STEPS 4 // fair to assume most matrices can be divided by 4.
#define FRAMETIME (T_SECOND / STEPS)
#define FRAMES (RANDOM_TIME * STEPS)
#define STEP_X (matrix_getx() / STEPS / 2)
#define STEP_Y (matrix_gety() / STEPS / 2)

static int modno;
static int step;
static int dir = 1;
static int frame;
static ulong nexttick;

int init(int moduleno) {
	if (matrix_getx() < (STEPS * 2))
		return 1;
	if (matrix_gety() < (STEPS * 2))
		return 1;

	modno = moduleno;
	return 0;
}

int draw(int argc, char* argv[]) {
	if (frame == 0)
		nexttick = utime();

	if (step == 0)
		dir = 1;
	if (step == 4)
		dir = -1;

	matrix_clear();
	RGB color = { .red = randn(255), .green = randn(255), .blue = randn(255) };
	RGB black = { .red = 0, .green = 0, .blue = 0 };
	byte off_x = step * STEP_X;
	byte off_y = step * STEP_Y;

	int mx = matrix_getx();
	int my = matrix_gety();
	matrix_fill(0 + off_x, 0 + off_y, mx - 1 - off_x, my - 1 - off_y, &color);
	matrix_fill(1 + off_x, 1 + off_y, mx - 2 - off_x, my - 2 - off_y, &black);

	matrix_render();
	if (frame >= FRAMES) {
		frame = 0;
		step = 0;
		return 1;
	}
	frame++;
	step += dir;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int deinit() {
	return 0;
}
