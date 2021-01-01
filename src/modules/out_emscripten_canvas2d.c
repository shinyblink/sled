// wasm (emscripten) canvas output.
//
// Copyright (c) 2020, fridtjof <fridtjof@das-labor.org>
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
#include <timers.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <emscripten.h>
#include <emscripten/html5.h>

static int matx;
static int maty;
static int buf_size;
static RGB* buf;

// called by render to output a frame
EM_JS(void, js_render, (int matx, int maty), {
	// prepare
	const img = new ImageData(new Uint8ClampedArray(window.sled_outbuf), matx, maty);
	// draw
	window.requestAnimationFrame((timestamp) => {
			window.sled_ctx.putImageData(img, 0, 0);
	});
})

int init(int moduleno __attribute__((unused)), char *argstr __attribute__((unused))) {
	// init the canvas context for drawing
	EM_ASM({
		window.sled_ctx = Module.canvas.getContext('2d', {
			alpha: true,
			antialias: false,
			depth: false
		});
	});

	// get canvas size and allocate an output buffer
	int em_ret = emscripten_get_canvas_element_size("#sled_output", &matx, &maty);

	if (em_ret != EMSCRIPTEN_RESULT_SUCCESS)
		return -1;

	buf_size = matx * maty * sizeof(RGB);
	buf = malloc(buf_size);

	// pass the buffer to JS for drawing
	EM_ASM({
		window.sled_outbuf = HEAPU8.subarray($0, $0 + $1);
	}, buf, buf_size);

	return 0;
}

int getx(int _modno) {
	return matx;
}
int gety(int _modno) {
	return maty;
}

static int matrix_ppos(int x, int y) {
	return (x + (y * matx));
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < matx);
	assert(y < maty);

	int pos = matrix_ppos(x, y);
	buf[pos] = color;
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < matx);
	assert(y < maty);

	int pos = matrix_ppos(x, y);
	return buf[pos];
}

int clear(int _modno) {
	memset(buf, 0, buf_size);
	return 0;
};

int render(int _modno) {
	js_render(matx, maty);
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
#ifdef CIMODE
	return desired_usec;
#else
	return timers_wait_until_core(desired_usec);
#endif
}

void wait_until_break(int _modno) {
#ifndef CIMODE
	timers_wait_until_break_core();
#endif
}

void deinit(int _modno) {
	free(buf);
}
