#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "text.h"

#define FRAMETIME (T_SECOND)
#define FRAMES (RANDOM_TIME)
#define PADX ((matrix_getx() - chars - 2) / 2)
#define CHARS_FULL 8 // 20:15:09
#define CHARS_SMALL 6 // 20:15

ulong nexttick, frametime;
int frame;
int moduleno;
// Boolean-map. This gets scrolled left, new text gets written on right.
int * text_buffer;
char clockstr[CHARS_FULL];
text* rendered = NULL;

int init(int modno) {
	moduleno = modno;

	if (matrix_getx() < 15)
		return 1; // not enough X to be usable
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable

	text_buffer = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	return 0;
}

int draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = utime();
	}

	time_t rawtime;
  struct tm * timeinfo;
	time (&rawtime);
  timeinfo = localtime (&rawtime);
	strftime(clockstr, CHARS_FULL, "%T", timeinfo);
	rendered = text_render(clockstr);
	if (!rendered)
		return 2;

	int x;
	int y;
	for (y = 0; y < matrix_gety(); y++)
				for (x = 0; x < matrix_getx(); x++) {
			int v = text_point(rendered, x, y) ? 255 : 0;
			RGB color = RGB(v, v, v);
			matrix_set(x, y, &color);
		}
	matrix_render();
	if (frame == FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, moduleno, 0, NULL);
	return 0;
}

int deinit() {
	text_free(rendered);
	free(text_buffer);
	return 0;
}
