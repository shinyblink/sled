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
#include <plugin.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

PGCTX_BEGIN_FILTER
	int scale;
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	if (!argstr) {
		eprintf("flt_scale: No scaling factor given.\n");
		free(ctx);
		return 2;
	}

	if (sscanf(argstr, "%d", &ctx->scale) == EOF) {
		eprintf("flt_scale: Couldn't parse argument as number: Got '%s'", argstr);
		free(ctx);
		free(argstr);
		return 2;
	}
	free(argstr);

	if (ctx->scale < 1) {
		eprintf("flt_scale: Scale factor must be greater equal 1.\n");
		free(ctx);
		return 1;
	}

	return 0;
}

int getx(int _modno) {
	PGCTX_GET
	return ctx->next->getx(ctx->nextid) / ctx->scale;
}
int gety(int _modno) {
	PGCTX_GET
	return ctx->next->gety(ctx->nextid) / ctx->scale;
}

int set(int _modno, int x, int y, RGB color) {
	PGCTX_GET
	int px = 0;
	int py = 0;
	int ret = 0;
	x = x * ctx->scale;
	y = y * ctx->scale;
	for (py = 0; py < ctx->scale; py++)
		for (px = 0; px < ctx->scale; px++) {
			ret = ctx->next->set(ctx->nextid, x + px, y + py, color);
			if (ret != 0) return ret;
		};
	return 0;
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	// Since we set all the pixels,
	// we know the scaled block will have the same colors.
	return ctx->next->get(ctx->nextid, x * ctx->scale, y * ctx->scale);
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
