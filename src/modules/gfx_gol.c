// Conway's Game of Life.
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
#include <random.h>
#include <stddef.h>
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define FRAMETIME (T_SECOND / 4) // 4fps, sounds goodish.
#define FRAMES (TIME_MEDIUM * 4)

#define ALIVE 1
#define DEAD 0

#define POS(x, y) (y * matrix_getx() + x)

static int modno;
static int frame;
static oscore_time nexttick;
static int* board;
static int* new;

static RGB white = RGB(255, 255, 255);
static RGB black = RGB(0, 0, 0);

int init(int moduleno, char* argstr) {
	// doesn't look very great with anything less.
	if (matrix_getx() < 8)
		return 1;
	if (matrix_gety() < 8)
		return 1;

	board = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	assert(board);
	new = malloc(matrix_gety() * matrix_getx() * sizeof(int));
	assert(new);

	modno = moduleno;
	frame = 0;
	return 0;
}

static void gol_shuffle_board(void) {
	int x;
	int y;
	for (x=0; x < matrix_getx(); ++x)
		for (y=0; y < matrix_gety(); ++y)
			board[POS(x, y)] = ((randn(8) == 0) ? ALIVE : DEAD);
}

void reset(int _modno) {
	nexttick = udate();
	gol_shuffle_board();
	frame = 0;
}

static int gol_adj(int x, int y) {
	int r;
	int c;
	int count = -board[POS(x, y)]; // if it's alive, substract one from the count.

	for (r = -1; r <= 1; ++r)
		for (c = -1; c <= 1; ++c) {
			int xp = x + r;
			int yp = y + c;
			if (xp >= matrix_getx()) xp -= matrix_getx();
			if (yp >= matrix_gety()) yp -= matrix_gety();
			if (xp < 0) xp += matrix_getx();
			if (yp < 0) yp += matrix_gety();

			if (board[POS(xp, yp)] == ALIVE)
				++count;
		};

	return count;
}

static void gol_cycle(void) {
	// Actual GoL rules.
	// 1) If a cell's neighbours are two, it'll keep it's state.
	// 2) If a cell's neighbours are three, it'll be alive, regardless of state.
	// 3) Any other count of neighbours will cause cells to die.;

	int x;
	int y;
	for (x = 0; x < matrix_getx(); ++x)
		for (y = 0; y < matrix_gety(); ++y) {
			int neighbours = gol_adj(x, y);

			if (neighbours == 2) {
				new[POS(x, y)] = board[POS(x, y)];
			} else if (neighbours == 3) {
				new[POS(x, y)] = ALIVE;
			} else {
				new[POS(x, y)] = DEAD;
			}
		}

	memcpy(board, new, matrix_getx() * matrix_gety() * sizeof(int));
}

int draw(int _modno, int argc, char* argv[]) {
	int x;
	int y;
	for (x=0; x < matrix_getx(); ++x)
		for (y=0; y < matrix_gety(); ++y)
			matrix_set(x, y, (board[POS(x, y)] == ALIVE) ? white : black);

	matrix_render();
	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	gol_cycle();
	return 0;
}

void deinit(int _modno) {
	free(board);
	free(new);
}
