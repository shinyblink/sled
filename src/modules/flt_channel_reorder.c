// Channel reorder.
// Switches channels around, if your matruc has different color order.
// Usage: -f channel_reorder:bgr
//
// Copyright (c) 2019, Draradech
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
#include <string.h>

PGCTX_BEGIN_FILTER
	char chan[3];
PGCTX_END

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER

	if (argstr && strlen(argstr) >= 3) {
		ctx->chan[0] = argstr[0];
		ctx->chan[1] = argstr[1];
		ctx->chan[2] = argstr[2];
	} else {
		ctx->chan[0] = 'r';
		ctx->chan[1] = 'g';
		ctx->chan[2] = 'b';
	}
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

int set(int _modno, int x, int y, RGB cin) {
	PGCTX_GET
	RGB cout;
	cout.red   = ctx->chan[0] == 'g' ? cin.green : (ctx->chan[0] == 'b' ? cin.blue : cin.red);
	cout.green = ctx->chan[1] == 'g' ? cin.green : (ctx->chan[1] == 'b' ? cin.blue : cin.red);
	cout.blue  = ctx->chan[2] == 'g' ? cin.green : (ctx->chan[2] == 'b' ? cin.blue : cin.red);
	cout.alpha = cin.alpha;
	return ctx->next->set(ctx->nextid, x, y, cout);
}

RGB get(int _modno, int x, int y) {
	PGCTX_GET
	RGB cin, cout;
	cout = ctx->next->get(ctx->nextid, x, y);
	cin.red   = ctx->chan[1] == 'r' ? cout.green : (ctx->chan[2] == 'r' ? cout.blue  : cout.red);
	cin.green = ctx->chan[0] == 'g' ? cout.red   : (ctx->chan[2] == 'g' ? cout.blue  : cout.green);
	cin.blue  = ctx->chan[0] == 'b' ? cout.red   : (ctx->chan[1] == 'b' ? cout.green : cout.blue);
	cin.alpha = cout.alpha;
	return cin;
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
