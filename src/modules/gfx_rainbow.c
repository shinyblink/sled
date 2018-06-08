// Rather simple rainbow.
// Uses HSV.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>

#define FRAMES 255
#define FRAMETIME ((RANDOM_TIME * T_SECOND) / 255)

static int modno;
static int pos;
static int frame = 0;
static ulong nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	return 0;
}

void reset(void) {
	nexttick = udate();
	frame = 0;
}

int draw(int argc, char* argv[]) {
	int x;
	int y;
	for (y = 0; y < matrix_gety(); ++y)
		for (x = 0; x < matrix_getx(); ++x) {
			RGB color = HSV2RGB(HSV(pos + x, 255, 255));
			matrix_set(x, y, color);
		}

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

int deinit(void) {
	return 0;
}
