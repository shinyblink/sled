// Filter that rotates by multiples of 90 degrees.

#include <types.h>
#include <timers.h>
#include <modloader.h>
#include <stdlib.h>

static module* next;
static int rot;

int init(int nextno, char* argstr) {
	// get next ptr.
	next = modules_get(nextno);
	rot = 1;
	if (argstr)
		rot = atoi(argstr) & 0x03;
	return 0;
}

int getx(void) {
	return next->getx();
}
int gety(void) {
	return next->gety();
}

int set(int x, int y, RGB color) {
	for (int i = 0; i < rot; i++) {
		int nx = getx() - 1 - x;
		x = y;
		y = nx;
	}
	return next->set(x, y, color);
}

RGB get(int x, int y) {
	for (int i = 0; i < rot; i++) {
		int nx = getx() - 1 - x;
		x = y;
		y = nx;
	}
	return next->get(x, y);
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
