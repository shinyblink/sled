// Rather simple sine drawing.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <math.h>
#include <stddef.h>
#include <graphics.h>

#define FRAMETIME (T_SECOND / 30)
#define FRAMES (RANDOM_TIME * 30)
#define HALF_Y (MATRIX_Y / 2)

static int modno;
static int pos;
static int frame;
static ulong nexttick;

byte lastx;
byte lasty;

int plugin_init(int moduleno) {
	if (MATRIX_Y < 3)
		return 1;
	modno = moduleno;
	return 0;
}

RGB white = { .red = 255, .green = 255, .blue = 255 };

int plugin_draw(int argc, char* argv[]) {
	if (frame == 0)
		nexttick = utime();

	matrix_clear();
	int x;
	int y;
	for (x = 0; x < MATRIX_X; ++x) {
		y = HALF_Y + (HALF_Y * sin((x + pos - 1) / M_PI));
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
	pos++;;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int plugin_deinit() {
	return 0;
}
