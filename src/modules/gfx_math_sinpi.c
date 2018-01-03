// Rather simple sine drawing.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <math.h>
#include <stddef.h>
#include <graphics.h>

#define FRAMETIME (T_SECOND / 30)
#define FRAMES (RANDOM_TIME * 30)
// Size of a full revolution
#define XSCALE ((float) matrix_getx())
#define HALF_Y (matrix_gety() / 2)

static int modno;
static int pos;
static int frame;
static ulong nexttick;

byte lastx;
byte lasty;

int init(int moduleno) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	return 0;
}

RGB white = { .red = 255, .green = 255, .blue = 255 };

int draw(int argc, char* argv[]) {
	if (frame == 0)
		nexttick = utime();

	matrix_clear();
	int x;
	int y;
	for (x = 0; x < matrix_getx(); ++x) {
		y = HALF_Y + (HALF_Y * sin((x + pos - 1) * M_PI * 2.0f / XSCALE));
		if (y < 0)
			y = 0;
		if (y >= matrix_gety())
			y = matrix_gety() - 1;
		matrix_set(x, y, &white);
		if (x != 0) graphics_drawline(lastx, lasty, x, y, &white);
		lastx = x;
		lasty = y;
	}
	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		pos = 0;
		return 1;
	}
	frame++;
	pos++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int deinit() {
	return 0;
}
