// Rather simple sine drawing.
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
#include <math.h>
#include <stddef.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>
#include <mathey.h>


#define FRAMETIME (T_SECOND / 30)
#define FRAMES (TIME_SHORT * 30)
// Size of a full revolution
#define XSCALE (matrix_getx())
#define YSCALE (matrix_gety())

static int modno;
static int pos;
static int frame;
static oscore_time nexttick;

int mx, my;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	frame = 0;

	mx = matrix_getx();
	my = matrix_gety();
	return 0;
}

RGB white = { .red = 255, .green = 255, .blue = 255 };

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
	pos = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	vec2 p, _p;
	int dy;

	matrix_clear();

	for (p.x = -1; p.x <= XSCALE; p.x++) {
		p.y = - round((YSCALE - 1) / 2 * (sin((p.x + pos) * M_PI * 2.0f / (double) XSCALE) - 1));

		if (p.x >= 0 && p.x < mx)
			if (p.y >= 0 && p.y < my)
				matrix_set(p.x, p.y, white);

		vec2 d = vadd(_p, vmul(p, -1));
		dy = round(d.y);
		d = vmul(d, 1.0/d.y);
		for (int i = 1; abs(dy)>1 && i<abs(dy); ++i) {
			int x = (int) ((p.x==0&&dy>0)?_p.x:p.x + i*d.x);
			int y = (int) (fmin(p.y,_p.y) + i*d.y);
			if (x >= 0 && x < mx)
				if (y >= 0 && y < my)
					matrix_set(x, y, white);
		}
		memcpy(&_p, &p, sizeof(p));
	}

	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}

	frame++;
	pos++;
	pos = pos % XSCALE;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
