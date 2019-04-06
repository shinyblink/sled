// Scaling filter.
// Does simple upscaling.
// No filtering or anything like that.
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
#include <stdio.h>
#include <assert.h>

static int scale = 0;
static int nextid;
static module* nextm;
static mod_flt* next;

int init(int moduleno, char* argstr) {
	nextid = ((mod_out*) mod_get(moduleno)->mod)->next;
	nextm = mod_get(nextid);
	next = nextm->mod;

	if (!argstr) {
		eprintf("flt_scale: No scaling factor given.\n");
		return 2;
	}

	if (sscanf(argstr, "%d", &scale) == EOF) {
		eprintf("flt_scale: Couldn't parse argument as number: Got '%s'", argstr);
		return 2;
	}
	free(argstr);

	if (scale < 1) {
		eprintf("flt_scale: Scale factor must be greater equal 1.\n");
		return 1;
	}

	return 0;
}

int getx(int _modno) {
	return next->getx(nextid) / scale;
}
int gety(int _modno) {
	return next->gety(nextid) / scale;
}

int set(int _modno, int x, int y, RGB color) {
	int px = 0;
	int py = 0;
	int ret = 0;
	x = x * scale;
	y = y * scale;
	for (py = 0; py < scale; py++)
		for (px = 0; px < scale; px++) {
			ret = next->set(nextid, x + px, y + py, color);
			if (ret != 0) return ret;
		};
	return 0;
}

RGB get(int _modno, int x, int y) {
	// Since we set all the pixels,
	// we know the scaled block will have the same colors.
	return next->get(nextid, x * scale, y * scale);
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

void deinit(int _modno) {
	return nextm->deinit(nextid);
}
