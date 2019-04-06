// Gamma correction filter.
// Corrects colors according to a generated LUT.
// Not the best, but it's better than nothing, I suppose.
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
#include <mod.h>
#include <math.h>

static int nextid;
static module* nextm;
static mod_flt* next;

#define GAMMA 2.8f
#define WHITEPOINT {0.98f, 1.0f, 1.0f} // R, G, B, respectively.

#define MAX_VAL 255
static byte LUT_R[MAX_VAL + 1];
static byte LUT_G[MAX_VAL + 1];
static byte LUT_B[MAX_VAL + 1];

#define CORRECTION ((powf((float)i / MAX_VAL, GAMMA) * MAX_VAL) + 0.5f)

int init(int moduleno, char* argstr) {
	// get next ptr.
	nextid = ((mod_out*) mod_get(moduleno)->mod)->next;
	nextm = mod_get(nextid);
	next = nextm->mod;
	float whitepoint[3] = WHITEPOINT;

	int i;
	for (i = 0; i <= 255; ++i)
		LUT_R[i] = whitepoint[0] * CORRECTION;
	for (i = 0; i <= 255; ++i)
		LUT_G[i] = whitepoint[1] * CORRECTION;
	for (i = 0; i <= 255; ++i)
		LUT_B[i] = whitepoint[2] * CORRECTION;
	return 0;
}

int getx(int _modno) {
	return next->getx(nextid);
}
int gety(int _modno) {
	return next->gety(nextid);
}

int set(int _modno, int x, int y, RGB color) {
	RGB corrected = RGB(LUT_R[color.red], LUT_G[color.green], LUT_B[color.blue]);
	return next->set(nextid, x, y, corrected);
}

// TODO: reverse LUT to get back semi-original values
// if we pass the corrected values to the set function,
// it doesn't have the same color it had before.
// every time that happens, it'll get visibly darker.
RGB get(int _modno, int x, int y) {
	return next->get(nextid, x, y);
}

int clear(int _modno) {
	return next->clear(nextid);
}

int render(void) {
	return next->render(nextid);
}

ulong wait_until(int _modno, ulong desired_usec) {
	return next->wait_until(nextid, desired_usec);
}

void wait_until_break(int _modno) {
	if (next && next->wait_until_break)
		return next->wait_until_break(nextid);
}

void deinit(int _modno) {
	return nextm->deinit(nextid);
}
