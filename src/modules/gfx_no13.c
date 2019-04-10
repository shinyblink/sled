// Draw a "No Article 13" banner.
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

// TODO: implement width of strokes, maybe fix the 13?

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <graphics.h>
#include <stdlib.h>

static int minsize = 16;
static int size_x;
static int size_y;
static int center_x;
static int center_y;
static int mindim;
static int half;
static int third;
static int quarter;

int init(int moduleno, char* argstr) {
	if ((size_x = matrix_getx()) < minsize)
		return 1; // not enough X to be looking good
	if ((size_y = matrix_gety()) < minsize)
		return 1; // not enough Y to be looking good

	mindim = MIN(size_x, size_y);
	half = mindim / 2;
	third = mindim / 3;
	quarter = mindim / 4;
	center_x = size_x / 2;
	center_y = size_y / 2;
	return 0;
}

static RGB signcol = RGB(255, 0, 0); // red X
static RGB textcol = RGB(255, 255, 255); // white text

int draw(int _modno, int argc, char* argv[]) {
	matrix_clear();
	// Draw the 1.
	int one_x = center_x - quarter;
	int one_size = half;
	graphics_drawline(one_x, center_y-(one_size/2), one_x, center_y+(one_size/2), textcol);

	// Draw the 3.
	int three_x = center_x+quarter;
	int three_size = half;
	int three_width = quarter;
	int three_wh = three_width / 2;
	int three_hsz = three_size / 2;
	graphics_drawline(three_x+three_wh, center_y-(three_size/2), three_x+three_wh, center_y+(three_size/2), textcol);
	int y_off = -three_hsz;
	int line;
	for (line = 0; line < 3; line++) {
		graphics_drawline(three_x-three_wh, center_y+y_off, three_x+three_wh, center_y+y_off, textcol);
		y_off += three_hsz;
	}

	// Draw the striked through circle.
	graphics_drawcircle(center_x, center_y, half, signcol);
	graphics_drawline(center_x+half, center_y-half, center_x-half, center_y+half, signcol);
	matrix_render();
	return 0;
}

void reset(int _modno) {
	// Nothing?
}

void deinit(int _modno) {}
