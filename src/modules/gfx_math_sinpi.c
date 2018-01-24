// Rather simple sine drawing.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <math.h>
#include <stddef.h>
#include <graphics.h>
#include <stdlib.h>

#define FRAMETIME (T_SECOND / 30)
#define FRAMES (RANDOM_TIME * 30)
// Size of a full revolution
#define XSCALE ((float) matrix_getx())
#define HALF_Y (matrix_gety() / 2)

static int modno;
static int pos;
static int frame;
static ulong nexttick;

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
	int mx = matrix_getx();
	int my = matrix_gety();
	int x;
	int y;
	int lastx = 0;
	int lasty = 0;
	for (x = 0; x < mx; ++x) {
		y = HALF_Y + (HALF_Y * sin((x + pos - 1) * M_PI * 2.0f / XSCALE));
		if (y < 0)
			y = 0;
		if (y >= my)
			y = my - 1;
		matrix_set(x, y, &white);
		if (x != 0) {
			matrix_set(x, lasty, &white);
			// fill gaps, slightly less ugly than invoking the line thing every call.
			if (abs(y - lasty) > 1) {
				int up = (lasty < y);
				int px = lastx + 1;
				int py = lasty + (up ? 1 : -1);
				matrix_set(px, py, &white);
				if ((px + 1) < mx) matrix_set(px + 1, py, &white);
			}
		};
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
