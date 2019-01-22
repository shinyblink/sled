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

static module* nextm;
static mod_flt* next;
static int rot;

int init(int nextno, char* argstr) {
	// get next ptr.
	nextm = mod_get(nextno);
	next = nextm->mod;
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
	return nextm->deinit(mod_getid(nextm));
}
