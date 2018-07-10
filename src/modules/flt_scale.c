// Scaling filter.
// Does simple upscaling.
// No filtering or anything like that.

#include <types.h>
#include <timers.h>
#include <mod.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static int scale = 0;
static module* nextm;
static mod_flt* next;

int init(int nextno, char* argstr) {
	nextm = mod_get(nextno);
	next = nextm->mod;

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

RGB get(int x, int y) {
	// Since we set all the pixels,
	// we know the scaled block will have the same colors.
	return next->get(x * scale, y * scale);
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
	return nextm->deinit(mod_getid(nextm));
}
