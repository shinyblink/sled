// "graphics", some helpers for matrix stuff.
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

#include "types.h"
#include "matrix.h"
#include <math.h>
#include "mathey.h"
#include <stdlib.h>

// PREV: replace this. 20kdc, looking at you.
// <20kdc> Doesn't need replacing - amazingly elegant. Changed to PREV to avoid flagging searches.
//         I did do some cleanup though, does that count?
//         Note that this uses signed ints so clipping is "automatic".
// (Later, 20kdc again: April 1st, changed this to use a callback. It still uses actual matrix bounds, though.
//  A define in graphics.h handles conversion to this API)

#define GRAPHICS_INBOUNDS(X, Y, CW, CH) (((((X) >= 0) && ((X) < (CW))) && (((Y) >= 0) && ((Y) < (CH)))))

int graphics_drawline_core(int x1, int y1, int x2, int y2, int (*set)(int, int, void*), void * ud) {
	int ret;
	int dx, dy; // D: Result of subtracting source from target
	int sdx, sdy; // Sign of D
	int dxabs, dyabs; // Absolute of D
	int x, y; // Subpixel position (0 to d*abs - 1)
	int px, py; // Position
	int sw = matrix_getx(), sh = matrix_gety(); // Cached matrix size

	dx = x2 - x1;	/* the horizontal distance of the line */
	dy = y2 - y1;	/* the vertical distance of the line */
	dxabs = abs(dx);
	dyabs = abs(dy);
	sdx = sign(dx);
	sdy = sign(dy);
	x = dyabs >> 1;
	y = dxabs >> 1;
	px = x1;
	py = y1;

	if (GRAPHICS_INBOUNDS(px, py, sw, sh)) {
		ret = set(px, py, ud);
		if (ret != 0) return ret;
	}

	if (dxabs >= dyabs) { /* the line is more horizontal than vertical */
		for(int i = 0; i < dxabs; ++i) {
			y += dyabs;
			if (y >= dxabs) {
				y -= dxabs;
				py += sdy;
			}
			px += sdx;
			if (GRAPHICS_INBOUNDS(px, py, sw, sh)) {
				ret = set(px, py, ud);
				if (ret != 0) return ret;
			}
		}
	} else { /* the line is more vertical than horizontal */
		for(int i = 0; i < dyabs; ++i) {
			x += dxabs;
			if (x >= dyabs) {
				x -= dyabs;
				px += sdx;
			}
			py += sdy;
			if (GRAPHICS_INBOUNDS(px, py, sw, sh)) {
				ret = set(px, py, ud);
				if (ret != 0) return ret;
			}
		}
	}
	return 0;
}

// Small helper.
int graphics_drawline_matrix(int x, int y, const RGB* col) {
	return matrix_set(x, y, *col);
}

// stolen from wikipedia because idk.
// *looking at previous comment from vif, raises eyebrows* - 20kdc
void graphics_drawcircle(int x0, int y0, byte radius, RGB color) {
	int x = radius - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (radius << 1);

	while (x >= y) {
		matrix_set(x0 + x, y0 + y, color);
		matrix_set(x0 + y, y0 + x, color);
		matrix_set(x0 - y, y0 + x, color);
		matrix_set(x0 - x, y0 + y, color);
		matrix_set(x0 - x, y0 - y, color);
		matrix_set(x0 - y, y0 - x, color);
		matrix_set(x0 + y, y0 - x, color);
		matrix_set(x0 + x, y0 - y, color);

		if (err <= 0) {
			y++;
			err += dy;
			dy += 2;
		}
		if (err > 0) {
			x--;
			dx += 2;
			err += dx - (radius << 1);
		}
	}
}
