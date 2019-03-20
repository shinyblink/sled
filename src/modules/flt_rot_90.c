// Filter that rotates by multiples of 90 degrees.
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
#include <mod.h>
#include <stdlib.h>

static int nextid;
static module* nextm;
static mod_flt* next;
static int rot;

int init(int moduleno, char* argstr) {
	// get next ptr.
	nextid = ((mod_out*) mod_get(moduleno)->mod)->next;
	nextm = mod_get(nextid);
	next = nextm->mod;
	rot = 1;
	if (argstr)
		rot = atoi(argstr) & 0x03;
	return 0;
}

int getx(int _modno) {
	return next->getx(nextid);
}
int gety(int _modno) {
	return next->gety(nextid);
}

int set(int _modno, int x, int y, RGB color) {
	for (int i = 0; i < rot; i++) {
		int nx = getx(0) - 1 - x;
		x = y;
		y = nx;
	}
	return next->set(nextid, x, y, color);
}

RGB get(int _modno, int x, int y) {
	for (int i = 0; i < rot; i++) {
		int nx = getx(0) - 1 - x;
		x = y;
		y = nx;
	}
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
	if (next->wait_until_break)
		return next->wait_until_break(nextid);
}

int deinit(int _modno) {
	return nextm->deinit(nextid);
}
