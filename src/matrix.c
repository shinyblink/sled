// The things actually doing the matrix manipulation.
// Also contains the buffers.
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
#include <string.h>
#include <assert.h>
#include "mod.h"
#include "main.h"

// This is where the matrix functions send output.
// It is the root of the output chain.
static int mod_out_no;
static module* out;

int matrix_init(int outmodno) {
	out = mod_get(outmodno);
	mod_out_no = outmodno;
	return 0;
}

int matrix_getx(void) {
	return out->getx(mod_out_no);
}
int matrix_gety(void) {
	return out->gety(mod_out_no);
}

int matrix_set(int x, int y, RGB color) {
	return out->set(mod_out_no, x, y, color);
}

RGB matrix_get(int x, int y) {
	return out->get(mod_out_no, x, y);
}

// Fills part of the matrix with jo-- a single color.
int matrix_fill(int start_x, int start_y, int end_x, int end_y, RGB color) {
	if (start_x > end_x)
		return 1;
	if (start_y > end_y)
		return 2;

	int x;
	int y;

	for (y = MAX(start_y, 0); y <= MIN(end_y, matrix_gety()); y++)
		for (x = MAX(start_x, 0); x <= MIN(end_x, matrix_getx()); x++) {
			matrix_set(x, y, color);
		}
	return 0;
}

// Zeroes the stuff.
int matrix_clear(void) {
	return out->clear(mod_out_no);
}

int matrix_render(void) {
	return out->render(mod_out_no);
}

int matrix_deinit(void) {
	return 0;
}
