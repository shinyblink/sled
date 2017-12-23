// "She sat down on the balcony of her tower, watching the stars sparkle."

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>

#define TWINKLE_LEVELS 8
#define TWINKLE_FRAMETIME 100 * T_MILLISECOND
#define TWINKLE_FRAMES RANDOM_TIME * 10
const int twinkle_level_tab[TWINKLE_LEVELS] = {
	0,
	32,
	192,
	224,
	255,
	192,
	128,
	64
};
static int twinkle_levels[MATRIX_X * MATRIX_Y];
static int twinkle_moduleno;
static ulong twinkle_nexttick;
int twinkle_framecount;

int plugin_init(int moduleno) {
	int i;
	for (i = 0; i < (MATRIX_X * MATRIX_Y); i++)
		twinkle_levels[i] = 0;
	twinkle_moduleno = moduleno;
	return 0;
}

int plugin_draw(int argc, char* argv[]) {
	if (twinkle_framecount == 0)
		twinkle_nexttick = utime();

	int x;
	int y;
	int i = 0;
	for (y = 0; y < MATRIX_Y; y++)
		for (x = 0; x < MATRIX_X; x++) {
			if (!twinkle_levels[i]) {
				if (randn(512) == 0)
					twinkle_levels[i] = 1;
			} else {
				twinkle_levels[i]++;
				twinkle_levels[i] %= TWINKLE_LEVELS;
			}
			int vB = twinkle_level_tab[twinkle_levels[i++]];
			int vG = (vB * 3) / 4;
			RGB color = { .red = vG, .green = vG, .blue = vB};
			matrix_set(x, y, &color);
		}

	matrix_render();
	if (twinkle_framecount >= TWINKLE_FRAMES) {
		twinkle_framecount = 0;
		return 1;
	}
	twinkle_framecount++;
	twinkle_nexttick += TWINKLE_FRAMETIME;
	timer_add(twinkle_nexttick, twinkle_moduleno, 0, NULL);
	return 0;
}

int plugin_deinit() {
	return 0;
}
