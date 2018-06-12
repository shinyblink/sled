// Debug filter.
// Does nothing but pass things through, but logging first.

#include <types.h>
#include <timers.h>
#include <modloader.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static int scale = 0;
static module* next;

int init(int nextno, char* argstr) {
	next = modules_get(nextno);

	if (!argstr) {
		eprintf("flt_scale: No scaling factor given.\n");
		return 2;
	}

	if (sscanf(argstr, "%d", &scale) == EOF) {
		eprintf("flt_scale: Couldn't parse argument as number: Got '%s'", argstr);
		return 2;
	}
	free(argstr);

	if (scale < 1) {
		eprintf("flt_scale: Scale factor must be greater equal 1.\n");
		return 1;
	}

	return 0;
}

int getx(void) {
	return next->getx() / scale;
}
int gety(void) {
	return next->gety() / scale;
}

int set(int x, int y, RGB color) {
	int px = 0;
	int py = 0;
	int ret = 0;
	x = x * scale;
	y = y * scale;
	for (py = 0; py < scale; py++)
		for (px = 0; px < scale; px++) {
			ret = next->set(x + px, y + py, color);
			if (ret != 0) return ret;
		};
	return 0;
}

int clear(void) {
	return next->clear();
}

int render(void) {
	return next->render();
}

ulong wait_until(ulong desired_usec) {
	return next->wait_until(desired_usec);
}

void wait_until_break(void) {
	if (next->wait_until_break)
		return next->wait_until_break();
}

int deinit(void) {
	return next->deinit();
}
