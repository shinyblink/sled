// Debug filter.
// Does nothing but pass things through, but logging first.

#include <types.h>
#include <timers.h>
#include <modloader.h>
#include <stdio.h>
#include <assert.h>

static module* next;

int init(int nextno) {
	printf("flt_dummy loading. next mod is %i\n", nextno);
	fflush(stdin);
	// get next ptr.
	next = modules_get(nextno);
	printf("next is %p", next);
	return 0;
}

int getx(void) {
	printf("getx\n");
	fflush(stdin);
	assert(next != NULL);
	return next->getx();
}
int gety(void) {
	printf("gety\n");
	fflush(stdin);
	assert(next != NULL);
	return next->gety();
}

int set(int x, int y, RGB *color) {
	printf("Setting (%i,%i).\n", x, y);
	assert(next != NULL);
	return next->set(x, y, color);
}

int clear(void) {
	printf("clear\n");
	fflush(stdin);
	assert(next != NULL);
	return next->clear();
}

int render(void) {
	printf("render\n");
	fflush(stdin);
	assert(next != NULL);
	return next->render();
}

ulong wait_until(ulong desired_usec) {
	printf("wait_until\n");
	fflush(stdin);
	assert(next != NULL);
	return next->wait_until(desired_usec);
}

int deinit(void) {
	printf("deinit\n");
	fflush(stdin);
	assert(next != NULL);
	return next->deinit();
}
