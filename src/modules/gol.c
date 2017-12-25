// Conway's Game of Life.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <random.h>
#include <string.h>

#define FRAMETIME (T_SECOND / 4) // 4fps, sounds goodish.
#define FRAMES (RANDOM_TIME * 4)

#define ALIVE 1
#define DEAD 0

#define BUFSZ (MATRIX_X * MATRIX_Y)
#define POS(x, y) (y * MATRIX_X + x)

static int modno;
static int frame;
static ulong nexttick;
static int board[BUFSZ];

RGB white = { .red = 255, .green = 255, .blue = 255 };

int plugin_init(int moduleno) {
	// doesn't look very great with anything less.
	if (MATRIX_X < 8)
		return 1;
	if (MATRIX_Y < 8)
		return 1;

	modno = moduleno;
	return 0;
}

void gol_shuffle_board(void) {
	int x;
	int y;
	for (x=0; x < MATRIX_X; ++x)
		for (y=0; y < MATRIX_Y; ++y)
			board[POS(x, y)] = ((randn(8) == 0) ? ALIVE : DEAD);
}

int gol_adj(int x, int y) {
	int r;
	int c;
	int count = -board[POS(x, y)]; // if it's alive, substract one from the count.

	for (r = -1; r <= 1; ++r)
		for (c = -1; c <= 1; ++c) {
			int xp = x + r;
			int yp = y + c;
			if (xp >= MATRIX_X) xp -= MATRIX_X;
			if (yp >= MATRIX_Y) yp -= MATRIX_Y;
			if (xp < 0) xp += MATRIX_X;
			if (yp < 0) xp += MATRIX_Y;

			if (board[POS(xp, yp)] == ALIVE)
				++count;
		};

	return count;
}

void gol_cycle(void) {
	// Actual GoL rules.
	// 1) If a cell's neighbours are two, it'll keep it's state.
	// 2) If a cell's neighbours are three, it'll be alive, regardless of state.
	// 3) Any other count of neighbours will cause cells to die.;

	int new[BUFSZ];
	int x;
	int y;
	for (x = 0; x < MATRIX_X; ++x)
		for (y = 0; y < MATRIX_Y; ++y) {
			int neighbours = gol_adj(x, y);

			if (neighbours == 2) {
				new[POS(x, y)] = board[POS(x, y)];
			} else if (neighbours == 3) {
				new[POS(x, y)] = ALIVE;
			} else {
				new[POS(x, y)] = DEAD;
			}
		}

	// TODO: replace with memcpy once not 5 am.
	for (x = 0; x < MATRIX_X; ++x)
		for (y = 0; y < MATRIX_Y; ++y)
			board[POS(x, y)] = new[POS(x, y)];
}

int plugin_draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = utime();
		gol_shuffle_board();
	}

	matrix_clear();
	int x;
	int y;
	for (x=0; x < MATRIX_X; ++x)
		for (y=0; y < MATRIX_Y; ++y)
			if (board[POS(x, y)] == ALIVE)
				matrix_set(x, y, &white);

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

int plugin_deinit() {
	return 0;
}
