// Filter that tiles a huge-x-having matrix into a snake-patterned smaller one, for more Y.
#include <types.h>
#include <timers.h>
#include <mod.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

static module* nextm;
static mod_flt* next;
static int mx, my;
static int folds, pane_x;
static bool pane_order = 0;

int init(int nextno, char* argstr) {
	// get next ptr.
	nextm = mod_get(nextno);
	next = nextm->mod;
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

	if (folds == 0) {
		eprintf("flt_smapper: A 0 fold number is invalid!"); 
	}

	/* When the fold number is negative, we reverse the pane order. */
	if (folds < 0) {
		folds = -folds;
		pane_order = 1;
	}

	pane_x = (mx / folds);

	return 0;
}

int getx(void) {
	return mx / folds;
}

int gety(void) {
	return my * folds;
}

int set(int x, int y, RGB color) {
	int nx = x;
	int ny = y;
	int paneno = pane_order ? (folds - (y / my) - 1) : (y / my);
	nx = (paneno * pane_x) + (paneno % 2 == 1 ? pane_x - x - 1 : x);
	ny = (paneno % 2 == 1 ? my - (y % my) - 1 : (y % my));
	return next->set(nx, ny, color);
}

RGB get(int x, int y) {
	int nx = x;
	int ny = y;
	int paneno = pane_order ? (folds - (y / my) - 1) : (y / my);
	nx = (paneno * pane_x) + (paneno % 2 == 1 ? pane_x - x - 1 : x);
	ny = (paneno % 2 == 1 ? my - (y % my) - 1 : (y % my));
	return next->get(nx, ny);
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
