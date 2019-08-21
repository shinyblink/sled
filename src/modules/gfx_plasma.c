// A plasma, kinda ported from borgware: https://github.com/das-labor/borgware-2d/blob/master/src/animations/fpmath_patterns.c
// Done so with permission from Christian Kroll. Many thanks!

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <math.h>
#include <stddef.h>
#include <graphics.h>
#include <stdlib.h>
#include <stdio.h>
#include <random.h>

#define FRAMETIME (T_SECOND / 60)
#define FRAMES (TIME_MEDIUM * 60)

#define SCALE 64 // it's a magic number, yeah.

static int modno;
static int pos;
static int frame;
static oscore_time nexttick;

static float *colbuf;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	colbuf = malloc(matrix_getx() * sizeof(float));
	return 0;
}

static float dist(float x0, float y0, float x1, float y1) {
	return sqrtf(((x0 - x1) * (x0 - x1)) + ((y0 - y1) * (y0 - y1)));
}

void reset(int _modno) {
	nexttick = udate();
	pos = randn(255);

	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	float plasma = 1.0f / 3.6f;
	float ccols = cosf(matrix_getx());
	float srows = sinf(matrix_gety());

	int i;
	for (i = 0; i < matrix_getx(); ++i) {
		colbuf[i] = sinf((((float) i) * plasma) + ((float) (pos * 0.05)));
	}

	float intermediary;
	byte res;
	int x;
	int y;
	for (x = 0; x < matrix_getx(); ++x)
		for (y = 0; y < matrix_gety(); ++y) {
			intermediary = sinf(dist(x, y, srows, ccols) * plasma);
			res = (colbuf[x] + intermediary + (float) 2) * SCALE; // clipping is wanted to get dark spots.
			RGB color = RGB(res, 0, 0);
			matrix_set(x, y, color);
		};
	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	pos++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(colbuf);
}
