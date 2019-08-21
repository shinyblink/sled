// A digital clock.
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "text.h"

#define FRAMETIME (T_SECOND)
#define FRAMES (TIME_SHORT)
#define CHARS_FULL 8 // 20:15:09, must also hold 20:15 (small)

static oscore_time nexttick;
static int frame = 0;
static int moduleno;
static int usesmall;
static char clockstr[CHARS_FULL + 1];
static text* rendered = NULL;

int init (int modno, char* argstr) {
	moduleno = modno;

	if (matrix_getx() < 15)
		return 1; // not enough X to be usable
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable
	// max digit size is 4, plus 3 for :, so 4 + 4 + 3 + 4 + 4 + 3 + 4 + 4 = 24 + 6 = 30
	usesmall = matrix_getx() < 30;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	time_t rawtime;
	struct tm * timeinfo;
	const char * format = "%T";
	if (usesmall)
		format = "%R";
	time (&rawtime);
	timeinfo = localtime (&rawtime);
	if (!strftime(clockstr, CHARS_FULL + 1, format, timeinfo)) {
		// Empty string OR undefined, so change it
		if (usesmall) {
			strcpy(clockstr, "??:??");
		} else {
			strcpy(clockstr, "??:??:??");
		}
	}

	if (rendered)
		text_free(rendered);
	rendered = text_render(clockstr);
	if (!rendered)
		return 2;

	int x;
	int y;
	int padX = 0;
	if (rendered)
		padX = (matrix_getx() - rendered->len) / 2;
	int padY = (matrix_gety() - 7) / 2;
	for (y = 0; y < matrix_gety(); y++)
		for (x = 0; x < matrix_getx(); x++) {
			byte v = text_point(rendered, x - padX, y - padY);
			RGB color = RGB(v, v, v);
			matrix_set(x, y, color);
		}
	matrix_render();
	if (frame == FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	// This acts conditionally on rendered being non-NULL.
	text_free(rendered);
}
