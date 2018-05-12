// Filter that tiles a huge-x-having matrix into a snake-patterned smaller one, for more Y.
#include <types.h>
#include <timers.h>
#include <modloader.h>
#include <stdlib.h>
#include <stdio.h>

static module* next;
static int mx, my;
static int folds, pane_x;

int init(int nextno, char* argstr) {
	// get next ptr.
	next = modules_get(nextno);
	mx = next->getx();
	my = next->gety();

	if (!argstr) {
		eprintf("flt_smapper: No folding factor given.\n");
		return 2;
	}

	if (sscanf(argstr, "%d", &folds) == EOF) {
		eprintf("flt_smapper: Couldn't parse argument as number: Got '%s'", argstr);
		return 2;
	}
	free(argstr);

	pane_x = (mx / folds);

	return 0;
}

int getx(void) {
	return mx / folds;
}

int gety(void) {
	return my * folds;
}

int set(int x, int y, RGB *color) {
	int nx = x;
	int ny = y;
	int paneno = y / my;
	nx = (paneno * pane_x) + (paneno % 2 == 1 ? pane_x - x - 1 : x);
	ny = (paneno % 2 == 1 ? my - (y % my) - 1 : (y % my));
	return next->set(nx, ny, color);
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
