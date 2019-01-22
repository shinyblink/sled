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

static module* nextm;
static mod_flt* next;

int init(int nextno, char* argstr) {
	printf("flt_dummy loading. next mod is %i\n", nextno);
	fflush(stdin);
	// get next ptr.
	nextm = mod_get(nextno);
	next = nextm->mod;
	printf("next is %p", next);

	if (argstr) {
		printf("got argstr: %s", argstr);
		free(argstr);
	}
	return 0;
}

int getx(void) {
	printf("getx\n");
	fflush(stdout);
	assert(next != NULL);
	return next->getx();
}
int gety(void) {
	printf("gety\n");
	fflush(stdout);
	assert(next != NULL);
	return next->gety();
}

int set(int x, int y, RGB color) {
	printf("Setting (%i,%i).\n", x, y);
	assert(next != NULL);
	return next->set(x, y, color);
}

RGB get(int x, int y) {
	printf("Getting color at (%i,%i).\n", x, y);
	assert(next != NULL);
	return next->get(x, y);
}

int clear(void) {
	printf("clear\n");
	fflush(stdout);
	assert(next != NULL);
	return next->clear();
}

int render(void) {
	printf("render\n");
	fflush(stdout);
	assert(next != NULL);
	return next->render();
}

ulong wait_until(ulong desired_usec) {
	printf("wait_until\n");
	fflush(stdout);
	assert(next != NULL);
	return next->wait_until(desired_usec);
}

void wait_until_break(void) {
	printf("wait_until_break\n");
	fflush(stdout);
	assert(next != NULL);
	if (next->wait_until_break)
		return next->wait_until_break();
}

int deinit(void) {
	printf("deinit\n");
	fflush(stdin);
	assert(next != NULL);
	return nextm->deinit(mod_getid(nextm));
}
