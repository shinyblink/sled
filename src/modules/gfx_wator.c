// Rough implementation of https://en.wikipedia.org/wiki/Wa-Tor
// Inspired by Android implementation https://github.com/aperifons/wa-tor
//
// Copyright (c) 2020, Sebastian "basxto" Riedel <git@basxto.de>
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

#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stdlib.h>

#define FRAMETIME (T_SECOND / 20)
#define FISH_REPRODUCE (6)
#define SHARK_REPRODUCE (5)
#define SHARK_STARVE (4)

static int modno;
static oscore_time nexttick;
static int *table;
static int *table_copy;
static int width;
static int height;
static int counter = SHARK_REPRODUCE * FISH_REPRODUCE;
static int frames = 0;
static int fishs = 0;
static int sharks = 0;

int init(int moduleno, char *argstr) {
	width = matrix_getx();
	height = matrix_gety();
	modno = moduleno;
	table = malloc(width * height * sizeof(int));
	table_copy = malloc(width * height * sizeof(int));
	return 0;
}

void reset(int _modno) {
	frames = 20 * 60 * 3; // 3min
	fishs = 0;
	sharks = 0;
	nexttick = udate();
	for (int i = 0; i < width * height; ++i) {
		int rand = randn(100);
		if (rand < 90)
			table[i] = 0;
		else if (rand > 98) {
			// 1% shark
			table[i] = 2;
			sharks++;
		} else {
			// 9% fish
			table[i] = 1;
			fishs++;
		}
		table_copy[i] = 0;
	}
}

// 0 both free
// 1 one has a fish
static int point_free(int x, int y) {
	if (y < 0)
		y = height + y;
	else
		y = y % height;
	if (x < 0)
		x = width + x;
	else
		x = x % width;
	int index = (y * width) + x;
	return table[index] | table_copy[index];
}

static void move_point(int oldx, int oldy, int x, int y) {
	int old = (oldy * width) + oldx;
	int new = (y * width) + x;
	table[old] = 0;
	table[new] = table_copy[old];
	table_copy[old] = 0;
	table_copy[new] = 0;
}

static void move_fishark() {
	int *tmp = table_copy;
	table_copy = table;
	table = tmp;

	counter--;
	if (counter < 0)
		counter = (SHARK_REPRODUCE * FISH_REPRODUCE) - 1;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int cell = table_copy[(y * width) + x];
			if (cell > (1 + SHARK_STARVE)) {
				table_copy[(y * width) + x] = 0;
				sharks--;
			} else if (cell > 0) {
				int x_new = x;
				int y_new = y;
				if (cell > 1) {
					int tries = randn(8) + 1;
					// find fish
					for(int i = -1; i < 2; ++i)
						for(int j = -1; j < 2; ++j)
							if(tries > 0 && point_free(x + i, y + j) == 1) {
								x_new = x + i;
								y_new = y + j;
								tries--;
							}
				}
				if (x_new == x && y_new == y) {
					if (cell > 1) {
						// let shark starve a bit
						table_copy[(y * width) + x]++;
					}
					int tries = randn(8) + 1;
					// find free cell
					for(int i = -1; i < 2; ++i)
						for(int j = -1; j < 2; ++j)
							if(tries > 0 && point_free(x + i, y + j) == 0) {
								x_new = x + i;
								y_new = y + j;
								tries--;
							}
				} else {
					// shark found a fish
					table_copy[(y * width) + x] = 2;
					fishs--;
				}
				if (y_new < 0)
					y_new = height + y_new;
				else
					y_new = y_new % height;
				if (x_new < 0)
					x_new = width + x_new;
				else
					x_new = x_new % width;
				move_point(x, y, x_new, y_new);
				// reproduce to old cell
				if (x != x_new && y != y_new) {
					if (cell == 1 && (counter % FISH_REPRODUCE) == 0) {
						table[(y * width) + x] = 1;
						fishs++;
					}
					if (cell > 1 && (counter % SHARK_REPRODUCE) == 0) {
						table[(y * width) + x] = 2;
						sharks++;
					}
				}
			}
		}
	}
}

int draw(int _modno, int argc, char *argv[]) {
	move_fishark();
	matrix_clear();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			RGB col = RGB(0, 0, 255);
			int index = (width * y) + x;
			if (table[index] == 1) {
				// fish
				col = RGB(0, 255, 0);
			} else if (table[index] > 1) {
				// shark with darker color if starving
				col = RGB(255 / (table[index] - 1), 0, 0);
			}
			matrix_set(x, y, col);
		}
	}
	matrix_render();
	frames--;
	if(frames <= 0 || fishs <= 0 || sharks <= 0){
		return 1;
	}
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(table);
	free(table_copy);
}
