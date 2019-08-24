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
#include <plugin.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

PGCTX_BEGIN_FILTER
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	printf("flt_dummy loading. next mod is %i\n", ctx->nextid);
	fflush(stdin);
	printf("next is %p", ctx->next);

	if (argstr) {
		printf("got argstr: %s", argstr);
		free(argstr);
	}
	return 0;
}

int getx(int _modno) {
	PGCTX_GET
	printf("getx\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	return ctx->next->getx(ctx->nextid);
}
int gety(int _modno) {
	PGCTX_GET
	printf("gety\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	return ctx->next->gety(ctx->nextid);
}

int set(int _modno, int x, int y, RGB color) {
	PGCTX_GET
	printf("Setting (%i,%i).\n", x, y);
	assert(ctx->next != NULL);
	return ctx->next->set(ctx->nextid, x, y, color);
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	printf("Getting color at (%i,%i).\n", x, y);
	assert(ctx->next != NULL);
	return ctx->next->get(ctx->nextid, x, y);
}

int clear(int _modno) {
	PGCTX_GET
	printf("clear\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	return ctx->next->clear(ctx->nextid);
}

int render(int _modno) {
	PGCTX_GET
	printf("render\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	return ctx->next->render(ctx->nextid);
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	PGCTX_GET
	printf("wait_until\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	return ctx->next->wait_until(ctx->nextid, desired_usec);
}

void wait_until_break(int _modno) {
	PGCTX_GET
	printf("wait_until_break\n");
	fflush(stdout);
	assert(ctx->next != NULL);
	if (ctx->next->wait_until_break)
		ctx->next->wait_until_break(ctx->nextid);
}

void deinit(int _modno) {
	printf("deinit\n");
	fflush(stdin);
	PGCTX_DEINIT
}
