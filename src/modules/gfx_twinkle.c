// "She sat down on the balcony of her tower, watching the stars sparkle."
// Contributed by 20kdc. First non-test/stupid module!!

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TWINKLE_LEVELS 16
#define TWINKLE_FRAMETIME (50 * T_MILLISECOND)
#define TWINKLE_FRAMES (TIME_LONG * 20)

#define TWINKLE_COL(v) RGB(((v) * 3) / 4, ((v) * 3) / 4, (v))
static const RGB twinkle_level_tab[TWINKLE_LEVELS] = {
	TWINKLE_COL(0),
	TWINKLE_COL(16),
	TWINKLE_COL(32),
	TWINKLE_COL(116),
	TWINKLE_COL(192),
	TWINKLE_COL(208),
	TWINKLE_COL(224),
	TWINKLE_COL(240),
	TWINKLE_COL(255),
	TWINKLE_COL(224),
	TWINKLE_COL(192),
	TWINKLE_COL(160),
	TWINKLE_COL(128),
	TWINKLE_COL(96),
	TWINKLE_COL(64),
	TWINKLE_COL(32)
};
static int *twinkle_levels;
static int twinkle_moduleno;
static oscore_time twinkle_nexttick;
static int twinkle_framecount = 0;

int init(int moduleno, char* argstr) {
	twinkle_levels = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	assert(twinkle_levels);
	reset(0);
	twinkle_moduleno = moduleno;
	return 0;
}

void reset(int _modno) {
	twinkle_nexttick = udate();
	twinkle_framecount = 0;
	memset(twinkle_levels, 0, matrix_getx() * matrix_gety() * sizeof(int));
}

int draw(int _modno, int argc, char* argv[]) {
	int x;
	int y;
	int i = 0;
	int endsoon = twinkle_framecount >= TWINKLE_FRAMES;
	int endnow = endsoon;
	for (x = 0; x < matrix_getx(); x++) {
		int lineactivity = 0;
		for (y = 0; y < matrix_gety(); y++) {
			if (!twinkle_levels[i]) {
				// This "curtain" effect is intentional.
				// Can't self-sustain because lineactivity only increases as we go through the line.
				if ((!endsoon) || (lineactivity > (matrix_getx() / 129))) {
					if (!(rand() & 511))
						twinkle_levels[i] = 1;
				}
			} else {
				lineactivity++;
				endnow = 0;
				twinkle_levels[i]++;
				twinkle_levels[i] %= TWINKLE_LEVELS;
			}
			matrix_set(x, y, twinkle_level_tab[twinkle_levels[i++]]);
		}
	}

	matrix_render();
	if (endnow) {
		twinkle_framecount = 0;
		return 1;
	}
	twinkle_framecount++;
	twinkle_nexttick += TWINKLE_FRAMETIME;
	timer_add(twinkle_nexttick, twinkle_moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(twinkle_levels);
}
