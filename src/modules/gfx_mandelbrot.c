// Simple mandelbrot zoomer.
// Far from perfect. But it works.
// Uses the escape time algorythm. Using HSV to color things.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <math.h>
#include <mathey.h>
#include <stdlib.h>
#include <stdio.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS)

#define ITERATIONS 255
#define MAXZOOM 5.0f

static int modno;
static int frame;
static ulong nexttick;

static int *iters;
static int mx;
static int my;

#define PPOS(x, y) (x + (y * my))

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;

	iters = malloc((mx * my) * sizeof(int));
	if (iters == NULL)
		return 2;

	modno = moduleno;
	return 0;
}

// simple scaling
static int rescale(int x, int maxn, int newn) {
	float f = (float) x / (float) maxn;
	return f * (float) newn;
}

#define SCALE(i, n) ((float) i / (float) n)

int draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = udate();
		matrix_clear();
	}

	int min;
	int max;

	int px;
	int py;

	for (py = 0; py < my; py++)
		for (px = 0; px < mx; px++) {
			float zoom = SCALE(frame, FRAMES) * MAXZOOM;
			float x0 = ((SCALE(px, mx) * 3.5f) - 2.5f) / zoom;
			float y0 = ((SCALE(py, my) * 2.0f) - 1.0f) / zoom;

			float x = 0;
			float y = 0;
			int i = 0;
			while ((x*x + y*y) <= (2*2) && i < ITERATIONS) {
				float xt = x*x - y*y + x0;
				y = 2 * x * y + y0;
				x = xt;
				i++;
				if (i < min) min = i;
				if (i > max) max = i;
			}

			iters[PPOS(px, py)] = i;
		}

	for (py = 0; py < my; py++)
		for (px = 0; px < mx; px++) {
			int i = iters[PPOS(px, py)];
			RGB col = RGB(0, 0, 0);
			if (i != (ITERATIONS)) {
				if (min == max) max = min + 1;
				byte scaled = rescale(i - min, max, 255);
				col = HSV2RGB(HSV(scaled, 255, 255));
			}
			matrix_set(px, py, &col);
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
	free(iters);
	return 0;
}
