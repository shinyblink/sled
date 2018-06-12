// Dummy output.

#include <types.h>
#include <timers.h>
#include <assert.h>

// Matrix size
#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif


int init(void) {
	// Dummy!
	return 0;
}

int getx(void) {
	return MATRIX_X;
}
int gety(void) {
	return MATRIX_Y;
}

int set(int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < MATRIX_X);
	assert(y < MATRIX_Y);

	// Setting pixels? Nah, we're good.
	return 0;
}

int clear(void) {
	// We're already clean for a month!
	return 0;
};

int render(void) {
	// Meh, don't feel like it.
	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
#ifdef CIMODE
	return desired_usec;
#else
	return wait_until_core(desired_usec);
#endif
}

void wait_until_break(void) {
#ifndef CIMODE
	wait_until_break_core();
#endif
}

int deinit(void) {
	// Can we just.. chill for a moment, please?
	return 0;
}
