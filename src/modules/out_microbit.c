// "I WILL MAKE THIS MICROBIT TWINKLE" - 20kdc
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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "types.h"
#include "timers.h"
#include "oscore.h"
#include "random.h"

// DO BE AWARE: This is designed for a 5x5 matrix size.
// If you enter in something bigger, it will show the top-left 5x5.

// we're limited on RAM (16kib), so let's not do anything stupid.
// store in the exact same form we're going to show to the user,
//  i.e. shades of red
static byte microbit_matrix[MATRIX_X * MATRIX_Y];
static void * microbit_thread(void * ctx);

int init(void) {
	// honestly there's no way to stop SLED anyway, let's leak the thread
	oscore_task_create("out_microbit", microbit_thread, NULL);
	return 0;
}

// Aliases for the various NRF51 pin states so nobody gets confused.
#define PINSTATE_OUTPUT 1
#define PINSTATE_DISCONNECT 2
#define PINSTATE_INPUT_PULLDOWN 6

// Before continuing, let's just be clear about the organization of the microbit matrix:
// Theoretically, it's three "rows" with a variable number of columns (9, 7, 9).
// In practice, go take a look at:
// https://github.com/microbit-foundation/microbit-reference-design/blob/master/PDF/Schematic%20Print/Schematic%20Prints.PDF
//
// COL pins (pin bits 4+) must be PULL-DOWN INPUTS to light. Should be DISCONNECTED INPUTS otherwise.
// ROW pins (pin bits 13+) must be HIGH OUTPUTS to light. Should be DISCONNECTED INPUTS otherwise.

// This maps LED index (where 1.1 is 0, 1.2 is 1, etc.) to XY in BCD
static const char microbit_remap[] = {
	// 1.
	0x00, 0x20, 0x40, 0x43, 0x33, 0x23, 0x13, 0x03, 0x12,
	// 2.
	0x42, 0x02, 0x22, 0x10, 0x30, 0x34, 0x14,
	0x00, 0x00, // excess (pads out to simplify lighting)
	// 3.
	0x24, 0x44, 0x04, 0x01, 0x11, 0x21, 0x31, 0x41, 0x32
};
static void microbit_light(int row, const char * remap, int minimum) {
	// Output drivers ON (specs promise this won't have any effect on non-OUTPUT pins)
	*((volatile int *) 0x50000508) = 0x0000E000;
	// -- end init --
	// Set ROW pins to DISCONNECTED INPUTS
	for (int r = 0; r < 3; r++)
		((volatile int *) 0x50000700)[13 + r] = PINSTATE_DISCONNECT;
	// Set COL pins 
	for (int c = 0; c < 9; c++) {
		int light = 0;
		int pos = remap[c];
		light = microbit_matrix[((pos >> 4) & 0xF) + ((pos & 0xF) * MATRIX_X)] >= minimum;
		((volatile int *) 0x50000700)[4 + c] = light ? PINSTATE_INPUT_PULLDOWN : PINSTATE_DISCONNECT;
	}
	// Activate the row
	((volatile int *) 0x50000700)[13 + row] = PINSTATE_OUTPUT;
}
static void microbit_update() {
	// time-based in the hope that with often enough yields, it won't act weird
	// shifted right to reduce 'quantumyness' from RTC 32768hz
	oscore_time tmr = udate() >> 6;

	// extract fields
	// for proper PoV, keep this at about 6 bits max?
	int row = tmr & 3;
	tmr >>= 2;
	int comparator = tmr & 0xF;
	tmr >>= 4;

	// comparator adjustment
	comparator = (comparator << 4) | 8;

	// row 3 (index 2 here) is dark for some reason and we have a spare row index
	// 'donate' it
	if (row == 3)
		row = 2;
	// total: 4 ctrl bits
	if (row != 3) {
		// before continuing, let's handle something here: the comparator needs a gamma curve.
		// really tiny values will seem massive otherwise.
		// however, a key note is that since this is the COMPARATOR,
		// the gamma curve must be applied in inverse.
		comparator = 0xFF - comparator;
		// comparator is inverted, run gamma calc. {
		int point40 = 12;
		int point80 = 55;
		int pointC0 = 135;

		int rangeBase;
		int rangeLength;
		int rangeOutStart;
		int rangeOutEnd;
		if (comparator < 0x40) {
			rangeBase = 0; rangeLength = 0x40;
			rangeOutStart = 0; rangeOutEnd = point40 - 1;
		} else if (comparator < 0x80) {
			rangeBase = 0x40; rangeLength = 0x40;
			rangeOutStart = point40; rangeOutEnd = point80 - 1;
		} else if (comparator < 0xC0) {
			rangeBase = 0x80; rangeLength = 0x40;
			rangeOutStart = point80; rangeOutEnd = pointC0 - 1;
		} else {
			rangeBase = 0xC0; rangeLength = 0x40;
			rangeOutStart = pointC0; rangeOutEnd = 0xFF;
		}
		// this is just a normal linear interpolation
		comparator = rangeOutStart + (((comparator - rangeBase) * (rangeOutEnd - rangeOutStart)) / rangeLength);
		// } un-invert the comparator now the curve is applied
		comparator = 0xFF - comparator;
		microbit_light(row, microbit_remap + (row * 9), comparator);
	}
}
static void * microbit_thread(void * ctx) {
	while (1) {
		microbit_update();
		oscore_task_yield();
	}
	return NULL;
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
	int total = 0;
	total += color.red;
	total += color.green;
	total += color.blue;
	total /= 3;
	microbit_matrix[x + (y * MATRIX_X)] = (byte) total;
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < MATRIX_X);
	assert(y < MATRIX_Y);
	byte v = microbit_matrix[x + (y * MATRIX_X)];
	return RGB(v, 0, 0);
}

int clear(int _modno) {
	memset(microbit_matrix, 0, sizeof(microbit_matrix));
	return 0;
};

int render(void) {
	// Right, so. What's going on here is that we don't want to double-buffer,
	//  because for all we know any byte of RAM used could be our last.
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	// bye!
	
}
