// Dummy output.

#include <types.h>
#include <timers.h>

int init(void) {
	// Dummy!
	return 0;
}

byte getx(void) {
	return MATRIX_X;
}
byte gety(void) {
	return MATRIX_Y;
}

int set(byte x, byte y, RGB *color) {
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
	return wait_until_core(desired_usec);
}

int deinit(void) {
	// Can we just.. chill for a moment, please?
	return 0;
}
