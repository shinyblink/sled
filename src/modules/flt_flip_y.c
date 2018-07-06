// Filter that flips X.

#include <types.h>
#include <timers.h>
#include <modloader.h>

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
	int ny = gety() - 1 - y;
	return next->set(x, ny, color);
}

RGB get(int x, int y) {
	int ny = gety() - 1 - y;
	return next->get(x, ny);
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
