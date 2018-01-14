#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <random.h>
#include <assert.h>

#include "text.h"

#define TEXT_DEFAULT "h@ck me hard github.com/vifino/sled thanks"
#define TEXT_DEFFRAMETIME (5000000 / matrix_getx())
// note that this rounds up in case of, say, 7
#define TEXT_MINH (((matrix_gety() + 1) / 2) - 4)
// "gap" of zeroes after text
#define TEXT_GAP matrix_getx()
ulong text_nexttick, text_frametime;
int text_position, text_moduleno;
// Boolean-map. This gets scrolled left, new text gets written on right.
int * text_buffer;

text* rendered = NULL;

int init(int moduleno) {
	text_position = 0;
	text_moduleno = moduleno;

	if (matrix_getx() < 8)
		return 1; // not enough X to be usable
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable

	text_buffer = malloc(matrix_getx() * matrix_gety() * sizeof(int));
	return 0;
}

int draw(int argc, char* argv[]) {
	if (argc != 0) {
		text_free(rendered);
		// this always sets involved values to 0 or a valid value
		if (text_render(argv[0]))
			return 1;
		text_position = 0;
	}
	int i;
	if (text_position == 0) {
		if (argc == 0) {
			rendered = text_render(TEXT_DEFAULT);
			if (!rendered)
				return 1;
		}
		// Presumably this would be calculated based on an optional parameter or defaulting to TEXT_DEFFRAMETIME.
		text_nexttick = utime();
		text_frametime = TEXT_DEFFRAMETIME;
		for (i = 0; i < (matrix_getx() * matrix_gety()); i++)
			text_buffer[i] = 0;
		// Add "center text & quit early" here
	} else if (text_position == (rendered->len + TEXT_GAP)) {
		text_position = 0;
		text_free(rendered);
		return 1;
	}
	// advance buffer
	for (i = 0; i < ((matrix_getx() * matrix_gety()) - 1); i++)
		text_buffer[i] = text_buffer[i + 1];

	int x;
	int y;
	for (y = 0; y < matrix_gety(); y++) {
		// setup rightmost pixel
		text_buffer[(y * matrix_getx()) + (matrix_getx() - 1)] = text_point(rendered, text_position, y);
		for (x = 0; x < matrix_getx(); x++) {
			int v = text_buffer[x + (y * matrix_getx())] ? 255 : 0;
			RGB color = RGB(v, v, v);
			matrix_set(x, y, &color);
		}
	}
	matrix_render();
	text_position++;
	text_nexttick += text_frametime;
	timer_add(text_nexttick, text_moduleno, 0, NULL);
	return 0;
}

int deinit() {
	text_free(rendered);
	free(text_buffer);
	return 0;
}
