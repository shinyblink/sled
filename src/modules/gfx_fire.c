/*
 * Copyright 2019 Piotr Esden-Tempski <piotr@esden.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define FIRE_LEVELS 256
#define FIRE_FRAMETIME (50 * T_MILLISECOND)
#define FIRE_FRAMES (TIME_MEDIUM * 20)

static RGB fire_palette_lut[FIRE_LEVELS];
static int *fire;
static int fire_moduleno;
static oscore_time fire_nexttick;
static int fire_framecount = 0;

void fire_palette_init(void);

int init(int moduleno, char* argstr) {
	fire_palette_init();
	fire = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	assert(fire);
	reset(0);
	fire_moduleno = moduleno;
	return 0;
}

void fire_palette_init(void)
{
	for (int i = 0; i < 32; ++i) {
		/* black to blue */
		fire_palette_lut[i + 0].red = 0;
		fire_palette_lut[i + 0].green = 0;
		fire_palette_lut[i + 0].blue = i << 1;

		/* blue to red */
		fire_palette_lut[i + 32].red = i << 3;
		fire_palette_lut[i + 32].green = 0;
		fire_palette_lut[i + 32].blue = 64 - (i << 1);

		/* red to yellow */
		fire_palette_lut[i + 64].red = 255;
		fire_palette_lut[i + 64].green = i << 3;
		fire_palette_lut[i + 64].blue = 0;

		/* yellow to white */
		fire_palette_lut[i + 96].red = 255;
		fire_palette_lut[i + 96].green = 255;
		fire_palette_lut[i + 96].blue = i << 2;

		fire_palette_lut[i + 128].red = 255;
		fire_palette_lut[i + 128].green = 255;
		fire_palette_lut[i + 128].blue = 64 + (i << 2);

		fire_palette_lut[i + 160].red = 255;
		fire_palette_lut[i + 160].green = 255;
		fire_palette_lut[i + 160].blue = 128 + (i << 2);

		fire_palette_lut[i + 192].red = 255;
		fire_palette_lut[i + 192].green = 255;
		fire_palette_lut[i + 192].blue = 192 + i;

		fire_palette_lut[i + 224].red = 255;
		fire_palette_lut[i + 224].green = 255;
		fire_palette_lut[i + 224].blue = 224 + i;
	}
}

void reset(int _modno) {
	fire_nexttick = udate();
	fire_framecount = 0;
	memset(fire, 0, matrix_getx() * matrix_gety());
}

int draw(int _modno, int argc, char* argv[]) {
	int x;
	int y;
	int w = matrix_getx();
	int h = matrix_gety();
	bool endsoon = fire_framecount >= FIRE_FRAMES;
	bool endnow = endsoon;

	/* Set random hotspots in the bottom line */
	int y_off = w * (h - 1);
	for (x = 0; x < w; x++) {
		/* Get random number when not ending soon. */
		int random = endsoon ? 0 : rand() % 32;
		if (random > 20) { /* set random full heat sparks */
			fire[y_off + x] = 255;
		} else {
			fire[y_off + x] = 0;
		}
	}

#if 0
	if (!endsoon) {
		/* create sparks */
		for (int i = 0; i < 10; i++) {
			int random_x = rand() % w;
			int random_y = (rand() % (h / 4)) + ((h / 4) * 3);
			fire[random_x + (random_y * w)] = 255;
		}
	}
#endif

	/* Advance fire by one frame. */
	int tmp;
	for (int y_off = w * (h - 1); y_off > 0; y_off -= w) {
		for (x = 0; x < w; x++) {
			if (x == 0) { /* leftmost column */
				tmp = fire[y_off];
				tmp += fire[y_off + 1];
				tmp += fire[y_off - w];
				tmp /= 3;
			} else if (x == w - 1) { /* rightmost column */
				tmp = fire[y_off + x];
				tmp += fire[y_off - w + x];
				tmp += fire[y_off + x - 1];
				tmp /= 3;
			} else {
				tmp = fire[y_off + x];
				tmp += fire[y_off + x + 1];
				tmp += fire[y_off + x - 1];
				tmp += fire[y_off - w + x];
				tmp >>= 2;
			}

			/* decay */
			if (tmp > 1) {
				tmp -= 1;
			}

			/* set new pixel value */
			fire[y_off - w + x] = tmp;
		}
	}

	/* Draw fire. */
	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			if (fire[x + (y * w)]) {
				endnow = false;
			}
			matrix_set(x, y, fire_palette_lut[fire[x + (y * w)]]);
		}
	}

	matrix_render();
	if (endnow) {
		fire_framecount = 0;
		return 1;
	}
	fire_framecount++;
	fire_nexttick += FIRE_FRAMETIME;
	timer_add(fire_nexttick, fire_moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(fire);
}
