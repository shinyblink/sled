// Filter that flips X.

#include <types.h>
#include <timers.h>
#include <mod.h>

static module* next;

int init(int nextno, char* argstr) {
	// get next ptr.
	next = modules_get(nextno);
	return 0;
}

int getx(void) {
	return next->getx();
}
int gety(void) {
	return next->gety();
}

int set(int x, int y, RGB color) {
	int nx = getx() - 1 - x;
	return next->set(nx, y, color);
}

RGB get(int x, int y) {
	int nx = getx() - 1 - x;
	return next->get(nx, y);
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
