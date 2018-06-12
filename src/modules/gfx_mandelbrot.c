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
#include <taskpool.h>

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

static volatile int min;
static volatile int max;
static oscore_mutex lock;

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

	lock = oscore_mutex_new();

	modno = moduleno;
	frame = 0;
	return 0;
}

// simple scaling
static int rescale(int x, int maxn, int newn) {
	float f = (float) x / (float) maxn;
	return f * (float) newn;
}

#define SCALE(i, n) ((float) i / (float) n)

void reset(void) {
	nexttick = udate();
	matrix_clear();
	frame = 0;
	min = 0;
	max = 0;
}

void drawrow(void* row) {
	int py = *(int*) row;
	int px;

	if (py < 0 || py >= mx) return;

	for (px = 0; px < mx; px++) {
		float zoom = SCALE(frame, FRAMES) * MAXZOOM;
		float x0 = ((SCALE(px, mx) * 3.5f) - 2.5f) / zoom - 0.8f;
		float y0 = ((SCALE(py, my) * 2.0f) - 1.0f) / zoom - 0.27f;

		float x = 0;
		float y = 0;
		int i = 0;
		while ((x*x + y*y) <= (2*2) && i < ITERATIONS) {
			float xt = x*x - y*y + x0;
			y = 2 * x * y + y0;
			x = xt;
			i++;
			// we could lock this here. but performance.
			// data races are okay here.
			//oscore_mutex_lock(lock);
			if (i < min) min = i;
			if (i > max) max = i;
			//oscore_mutex_unlock(lock);
		}

		RGB col = RGB(0, 0, 0);
		if (i != (ITERATIONS)) {
			//oscore_mutex_lock(lock);
			if (min == max) max = min + 1;
			int mmin = min;
			int mmax = max;
			//oscore_mutex_unlock(lock);
			byte scaled = rescale(i - mmin, mmax, 255);
			col = HSV2RGB(HSV(scaled, 255, 255));
		}
		matrix_set(px, py, col);
	}
}

int draw(int argc, char* argv[]) {
	int py;

	taskpool_forloop(TP_GLOBAL, &drawrow, 0, my);

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

int deinit(void) {
	free(iters);
	return 0;
}
