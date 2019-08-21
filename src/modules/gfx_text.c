// Draws scrolling text.
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
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <random.h>
#include <assert.h>

#include "text.h"

#define TEXT_DEFAULT "h@ck me hard github.com/vifino/sled thanks"
#define TEXT_DEFFRAMETIME (5000000 / matrix_getx())
// note that this rounds up in case of, say, 7
#define TEXT_MINH (((matrix_gety() + 1) / 2) - 4)
// "gap" of zeroes after text
#define TEXT_GAP matrix_getx()

static oscore_time text_nexttick, text_frametime;
static int text_position, text_moduleno;

static text* rendered = NULL;

int init(int moduleno, char* argstr) {
	text_position = 0;
	text_moduleno = moduleno;

	if (matrix_getx() < 8)
		return 1; // not enough X to be usable
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable

	return 0;
}

void reset(int _modno) {
	text_free(rendered);
	text_position = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	if (argc != 0) {
		text_free(rendered);
		// this always sets involved values to 0 or a valid value.
		text_position = 0;
		rendered = text_render(argv[0]);
		if (!rendered)
			return 1;
	}
	if (text_position == 0) {
		if (argc == 0) {
			rendered = text_render(TEXT_DEFAULT);
			if (!rendered)
				return 1;
		}
		// Presumably this would be calculated based on an optional parameter or defaulting to TEXT_DEFFRAMETIME.
		text_nexttick = udate();
		text_frametime = TEXT_DEFFRAMETIME;
		// Add "center text & quit early" here
	} else if (text_position == (rendered->len + TEXT_GAP)) {
		text_position = 0;
		text_free(rendered);
		rendered = NULL;
		return 1;
	}

	int x;
	int y;
	for (y = 0; y < matrix_gety(); y++) {
		for (x = 0; x < matrix_getx(); x++) {
			byte v = text_point(rendered, (x + text_position) - matrix_getx(), y);
			RGB color = RGB(v, v, v);
			matrix_set(x, y, color);
		}
	}
	matrix_render();
	text_position++;
	text_nexttick += text_frametime;
	timer_add(text_nexttick, text_moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	// This acts conditionally on rendered being non-NULL.
	text_free(rendered);
}
