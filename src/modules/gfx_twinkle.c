// "She sat down on the balcony of her tower, watching the stars sparkle."
// Contributed by 20kdc. First non-test/stupid module!!

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>
#include <stdlib.h>
#include <assert.h>

#define TWINKLE_LEVELS 8
#define TWINKLE_FRAMETIME 100 * T_MILLISECOND
#define TWINKLE_FRAMES RANDOM_TIME * 10
static const int twinkle_level_tab[TWINKLE_LEVELS] = {
	0,
	32,
	192,
	224,
	255,
	192,
	128,
	64
};
static int *twinkle_levels;
static int twinkle_moduleno;
static ulong twinkle_nexttick;
int twinkle_framecount;

int init(int moduleno, char* argstr) {
	int i;
	twinkle_levels = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	assert(twinkle_levels);
	for (i = 0; i < (matrix_getx() * matrix_gety()); i++)
		twinkle_levels[i] = 0;
	twinkle_moduleno = moduleno;
	return 0;
}

int draw(int argc, char* argv[]) {
	if (twinkle_framecount == 0)
		twinkle_nexttick = udate();

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
				if ((!endsoon) || (lineactivity > (matrix_getx() / 129)))
					if (randn(512) == 0)
						twinkle_levels[i] = 1;
			} else {
				lineactivity++;
				endnow = 0;
				twinkle_levels[i]++;
				twinkle_levels[i] %= TWINKLE_LEVELS;
			}
			int vB = twinkle_level_tab[twinkle_levels[i++]];
			int vG = (vB * 3) / 4;
			RGB color = { .red = vG, .green = vG, .blue = vB};
			matrix_set(x, y, &color);
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

int deinit() {
	free(twinkle_levels);
	return 0;
}
