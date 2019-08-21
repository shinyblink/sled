// Dummy output.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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

int getx(int _modno) {
	return MATRIX_X;
}
int gety(int _modno) {
	return MATRIX_Y;
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < MATRIX_X);
	assert(y < MATRIX_Y);

	// Setting pixels? Nah, we're good.
	return 0;
}

RGB get(int _modno, int x, int y) {
	// Nice. We're batman.
	return RGB(0, 0, 0);
}

int clear(int _modno) {
	// We're already clean for a month!
	return 0;
};

int render(void) {
	// Meh, don't feel like it.
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
#ifdef CIMODE
	return desired_usec;
#else
	return timers_wait_until_core(desired_usec);
#endif
}

void wait_until_break(int _modno) {
#ifndef CIMODE
	timers_wait_until_break_core();
#endif
}

void deinit(int _modno) {
	// Can we just.. chill for a moment, please?
}
