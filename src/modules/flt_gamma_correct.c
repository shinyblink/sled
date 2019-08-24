// Gamma correction filter.
// Corrects colors according to a generated LUT.
// Not the best, but it's better than nothing, I suppose.
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
#include <plugin.h>
#include <math.h>

#define GAMMA 2.8f
#define WHITEPOINT {0.98f, 1.0f, 1.0f} // R, G, B, respectively.

#define MAX_VAL 255
PGCTX_BEGIN_FILTER
	byte LUT_R[MAX_VAL + 1];
	byte LUT_G[MAX_VAL + 1];
	byte LUT_B[MAX_VAL + 1];
PGCTX_END

#define CORRECTION ((powf((float)i / MAX_VAL, GAMMA) * MAX_VAL) + 0.5f)

int init(int _modno, char* argstr) {
	PGCTX_INIT_FILTER
	float whitepoint[3] = WHITEPOINT;

	int i;
	for (i = 0; i <= MAX_VAL; ++i) {
		ctx->LUT_R[i] = whitepoint[0] * CORRECTION;
		ctx->LUT_G[i] = whitepoint[1] * CORRECTION;
		ctx->LUT_B[i] = whitepoint[2] * CORRECTION;
    }
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
	RGB corrected = RGB(ctx->LUT_R[color.red], ctx->LUT_G[color.green], ctx->LUT_B[color.blue]);
	return ctx->next->set(ctx->nextid, x, y, corrected);
}

// TODO: reverse LUT to get back semi-original values
// if we pass the corrected values to the set function,
// it doesn't have the same color it had before.
// every time that happens, it'll get visibly darker.
RGB get(int _modno, int x, int y) {
	PGCTX_GET
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
	if (ctx->next && ctx->next->wait_until_break)
		return ctx->next->wait_until_break(ctx->nextid);
}

void deinit(int _modno) {
	PGCTX_DEINIT
}
