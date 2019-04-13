// Small module that randomly sets a static color.
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
#include <stdio.h>
#include <random.h>

int init(int moduleno, char* argstr) {
	return 0;
}

int draw(int _modno, int argc, char* argv[]) {
	RGB color = RGB(randn(220), randn(220), randn(220));

	matrix_fill(0, 0, matrix_getx() - 1, matrix_gety() - 1, color);

	matrix_render();
	return 0;
}

void reset(int _modno) {
	// Nothing?
}

void deinit(int _modno) {}
