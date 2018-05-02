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
#define XSCALE (matrix_getx())
#define HALF_Y (matrix_gety() / 2)

static int modno;
static int pos;
static int frame;
static ulong nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	frame = 0;
	return 0;
}

RGB white = { .red = 255, .green = 255, .blue = 255 };

void reset() {
	nexttick = udate();
	frame = 0;
}

int draw(int argc, char* argv[]) {
	matrix_clear();
	int mx = matrix_getx();
	int x;
	int y;
	int lasty = 0;
	int dy;
	int py;
	for (x = 0; x < mx; x++) {
		y = HALF_Y + ((HALF_Y - 1) * sin((x + pos) * M_PI * 2.0f / (float) XSCALE)); // * M_PI * 2.0f
		matrix_set(x, y, &white);
		if (x != 0) {
			matrix_set(x, lasty, &white);
			/* fill the gaps with a magnificent method */
			if ((dy = abs(y - lasty)) > 1) {
				for (py = fmin(y, lasty); py<fmin(y, lasty)+dy; py++)
					matrix_set(x, py, &white);
			}
		}
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
	pos = pos % XSCALE;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int deinit() {
	return 0;
}
