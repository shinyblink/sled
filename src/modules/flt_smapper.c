// Filter that splits a very wide matrix into a snake-patterned smaller one,
// for more Y size.
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
#include <stdbool.h>

PGCTX_BEGIN_FILTER
	int mx, my;
	int folds, pane_x;
	int pane_order;
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	// get next ptr.
	ctx->mx = ctx->next->getx(ctx->nextid);
	ctx->my = ctx->next->gety(ctx->nextid);
	ctx->pane_order = 0;

	if (!argstr) {
		eprintf("flt_smapper: No folding factor given.\n");
		free(argstr);
		free(ctx);
		return 2;
	}

	if (sscanf(argstr, "%d", &ctx->folds) == EOF) {
		eprintf("flt_smapper: Couldn't parse argument as number: Got '%s'", argstr);
		free(argstr);
		free(ctx);
		return 2;
	}
	free(argstr);

	if (ctx->folds == 0) {
		eprintf("flt_smapper: A 0 fold number is invalid!");
		free(ctx);
		return 2;
	}

	/* When the fold number is negative, we reverse the pane order. */
	if (ctx->folds < 0) {
		ctx->folds = -ctx->folds;
		ctx->pane_order = 1;
	}

	ctx->pane_x = (ctx->mx / ctx->folds);

	return 0;
}

int getx(int _modno) {
	PGCTX_GET
	return ctx->mx / ctx->folds;
}

int gety(int _modno) {
	PGCTX_GET
	return ctx->my * ctx->folds;
}

int set(int _modno, int x, int y, RGB color) {
	PGCTX_GET
	int nx = x;
	int ny = y;
	int paneno = ctx->pane_order ? (ctx->folds - (y / ctx->my) - 1) : (y / ctx->my);
	nx = (paneno * ctx->pane_x) + (paneno % 2 == 1 ? ctx->pane_x - x - 1 : x);
	ny = (paneno % 2 == 1 ? ctx->my - (y % ctx->my) - 1 : (y % ctx->my));
	return ctx->next->set(ctx->nextid, nx, ny, color);
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	int nx = x;
	int ny = y;
	int paneno = ctx->pane_order ? (ctx->folds - (y / ctx->my) - 1) : (y / ctx->my);
	nx = (paneno * ctx->pane_x) + (paneno % 2 == 1 ? ctx->pane_x - x - 1 : x);
	ny = (paneno % 2 == 1 ? ctx->my - (y % ctx->my) - 1 : (y % ctx->my));
	return ctx->next->get(ctx->nextid, nx, ny);
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
