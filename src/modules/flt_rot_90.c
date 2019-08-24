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
#include <plugin.h>
#include <stdlib.h>

PGCTX_BEGIN_FILTER
	int rot, mx, my;
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	ctx->rot = 1;
	if (argstr) {
		ctx->rot = atoi(argstr) & 0x03;
		free(argstr);
	}
	ctx->mx = ctx->next->getx(ctx->nextid);
	ctx->my = ctx->next->gety(ctx->nextid);
	return 0;
}

int getx(int _modno) {
	PGCTX_GET
	return (ctx->rot & 1) ? ctx->my : ctx->mx;
}
int gety(int _modno) {
	PGCTX_GET
	return (ctx->rot & 1) ? ctx->mx : ctx->my;
}

#define COORD_TRANSFORM \
	for (int i = 0; i < ctx->rot; i++) { \
		int inv = ((i & 1) ^ (ctx->rot & 1)) ? ctx->my : ctx->mx; \
		int nx = (inv - 1) - x; \
		x = y; \
		y = nx; \
	}

int set(int _modno, int x, int y, RGB color) {
	PGCTX_GET
	COORD_TRANSFORM
	return ctx->next->set(ctx->nextid, x, y, color);
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	COORD_TRANSFORM
	return ctx->next->get(ctx->nextid, x, y);
}

int clear(int _modno) {
	PGCTX_GET
	return ctx->next->clear(ctx->nextid);
}

int render(int _modno) {
	PGCTX_GET
	return ctx->next->render(ctx->nextid);
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	PGCTX_GET
	return ctx->next->wait_until(ctx->nextid, desired_usec);
}

void wait_until_break(int _modno) {
	PGCTX_GET
	if (ctx->next->wait_until_break)
		ctx->next->wait_until_break(ctx->nextid);
}

void deinit(int _modno) {
	PGCTX_DEINIT
}
