// Rather simple rectangle animation.
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
#include <random.h>
#include <stddef.h>

#define STEPS 4 // fair to assume most matrices can be divided by 4.
#define FRAMETIME (T_SECOND / STEPS)
#define FRAMES (TIME_SHORT * STEPS)
#define STEP_X (matrix_getx() / STEPS / 2)
#define STEP_Y (matrix_gety() / STEPS / 2)

static int modno;
static int step = 0;
static int dir = 1;
static int frame = 0;
static oscore_time nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < (STEPS * 2))
		return 1;
	if (matrix_gety() < (STEPS * 2))
		return 1;

	modno = moduleno;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
	step = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	if (step == 0)
		dir = 1;
	if (step == 4)
		dir = -1;

	matrix_clear();
	RGB color = RGB(randn(255), randn(255), randn(255));
	RGB black = RGB(0, 0, 0);
	byte off_x = step * STEP_X;
	byte off_y = step * STEP_Y;

	int mx = matrix_getx();
	int my = matrix_gety();
	matrix_fill(0 + off_x, 0 + off_y, mx - 1 - off_x, my - 1 - off_y, color);
	matrix_fill(1 + off_x, 1 + off_y, mx - 2 - off_x, my - 2 - off_y, black);

	matrix_render();
	if (frame >= FRAMES) {
		frame = 0;
		step = 0;
		return 1;
	}
	frame++;
	step += dir;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
