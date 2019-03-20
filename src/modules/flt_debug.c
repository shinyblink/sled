// Debug filter.
// Does nothing but pass things through, but logging first.
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
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

static int nextid;
static module* nextm;
static mod_flt* next;

int init(int moduleno, char* argstr) {
	printf("flt_dummy loading. next mod is %i\n", nextno);
	fflush(stdin);
	// get next ptr.
	nextid = ((mod_out*) mod_get(moduleno)->mod)->next;
	nextm = mod_get(nextid);
	next = nextm->mod;
	printf("next is %p", next);

	if (argstr) {
		printf("got argstr: %s", argstr);
		free(argstr);
	}
	return 0;
}

int getx(int _modno) {
	printf("getx\n");
	fflush(stdout);
	assert(next != NULL);
	return next->getx(nextid);
}
int gety(int _modno) {
	printf("gety\n");
	fflush(stdout);
	assert(next != NULL);
	return next->gety(nextid);
}

int set(int _modno, int x, int y, RGB color) {
	printf("Setting (%i,%i).\n", x, y);
	assert(next != NULL);
	return next->set(nextid, x, y, color);
}

RGB get(int _modno, int x, int y) {
	printf("Getting color at (%i,%i).\n", x, y);
	assert(next != NULL);
	return next->get(nextid, x, y);
}

int clear(int _modno) {
	printf("clear\n");
	fflush(stdout);
	assert(next != NULL);
	return next->clear(nextid);
}

int render(void) {
	printf("render\n");
	fflush(stdout);
	assert(next != NULL);
	return next->render(nextid);
}

ulong wait_until(int _modno, ulong desired_usec) {
	printf("wait_until\n");
	fflush(stdout);
	assert(next != NULL);
	return next->wait_until(nextid, desired_usec);
}

void wait_until_break(int _modno) {
	printf("wait_until_break\n");
	fflush(stdout);
	assert(next != NULL);
	if (next->wait_until_break)
		return next->wait_until_break(nextid);
}

int deinit(int _modno) {
	printf("deinit\n");
	fflush(stdin);
	assert(next != NULL);
	return nextm->deinit(nextid);
}
