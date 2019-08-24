// Simple checkerboard animation.
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
#include <matrix.h>
#include <timers.h>
#include <stddef.h>

#define FPS 2
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 2)
		return 1;
	if (matrix_gety() < 2)
		return 1;

	modno = moduleno;
	frame = 0;
	return 0;
}

static RGB white = RGB(255, 255, 255);
static RGB black = RGB(0, 0, 0);

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	frame = 0;
}


int draw(int _modno, int argc, char* argv[]) {
	int mx = matrix_getx();
	int my = matrix_gety();

	int x;
	int y;
	for (y = 0; y < my; y++)
		for (x = 0; x < mx; x++)
			matrix_set(x, y, ((y + x + frame) % 2) ? black : white);

	matrix_render();
	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
