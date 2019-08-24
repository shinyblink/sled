// Filter that flips X.
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

PGCTX_BEGIN_FILTER
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	free(argstr);
	return 0;
}

int getx(int _modno) {
	PGCTX_GET
	return ctx->next->getx(ctx->nextid);
}
int gety(int _modno) {
	PGCTX_GET
	return ctx->next->gety(ctx->nextid);
}

int set(int _modno, int x, int y, RGB color) {
	PGCTX_GET
	int nx = getx(_modno) - 1 - x;
	return ctx->next->set(ctx->nextid, nx, y, color);
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	int nx = getx(_modno) - 1 - x;
	return ctx->next->get(ctx->nextid, nx, y);
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
		return ctx->next->wait_until_break(ctx->nextid);
}

void deinit(int _modno) {
	PGCTX_DEINIT
}
