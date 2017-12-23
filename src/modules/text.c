#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <stdlib.h>
#include <random.h>

#define TEXT_FRAMETIME 100000
// note that this rounds up in case of, say, 7
#define TEXT_MINH (((MATRIX_Y + 1) / 2) - 4)
// "gap" of zeroes after text
#define TEXT_GAP MATRIX_X
ulong text_nexttick;
int text_position, text_queuedrender_len, text_moduleno;
// Boolean-map. This gets scrolled left, new text gets written on right.
int text_buffer[MATRIX_X * MATRIX_Y];
byte * text_queuedrender;

#include "font.h"

int plugin_init(int moduleno) {
	text_position = 0;
	text_queuedrender_len = 0;
	text_queuedrender = 0;
	text_moduleno = moduleno;
	if (MATRIX_X < 8)
		return 1; // not enough X to be usable
	if (MATRIX_Y < 7)
		return 1; // not enough Y to be usable
	return 0;
}

int text_point(int y) {
	if (y < TEXT_MINH)
		return 0;
	if (y >= TEXT_MINH + 8)
		return 0;
	if (text_position >= text_queuedrender_len)
		return 0;
	if (!text_queuedrender)
		return 0;
	return text_queuedrender[text_position] & (1 << (y - TEXT_MINH));
}

int text_render_core(char * txt, byte * outp) {
	int columns = 0;
	int first = 1;
	while (*txt) {
		int space = 0;
		if (!first)
			space = 1;
		first = 0;
		byte * fontptr = font_data;
		byte txtb = (byte) (*txt);
		if (txtb == 0x20)
			space = 3;
		while (space--) {
			columns++;
			if (outp) {
				*outp = 0;
				outp++;
			}
		}
		if ((txtb == 0x20) || (txtb >= 0x80)) {
			txt++;
			continue;
		}
		if (txtb >= 0x40) {
			// Page 2
			fontptr += 256;
		}
		int loc = font_lookup[txtb];
		while (loc != 0x100) {
			if (fontptr[loc] == 0xFF)
				break;
			columns++;
			if (outp) {
				*outp = fontptr[loc];
				outp++;
			}
			loc++;
		}
		txt++;
	}
	return columns;
}
int text_render(char * txt) {
	text_queuedrender_len = text_render_core(txt, 0);
	text_queuedrender = malloc(text_queuedrender_len);
	if (!text_queuedrender)
		return 1;
	text_render_core(txt, text_queuedrender);
	return 0;
}

int plugin_draw(int argc, char* argv[]) {
	if (argc != 0) {
		if (text_queuedrender)
			free(text_queuedrender);
		// this always sets it to 0
		if (text_render(argv[0]))
			return 1;
		text_position = 0;
	}
	int i;
	if (text_position == 0) {
		if (argc == 0)
			if (text_render("anyway, I'll be ready to push text.so, but I ought to get some sample text first"))
				return 1;
		text_nexttick = utime();
		for (i = 0; i < (MATRIX_X * MATRIX_Y); i++)
			text_buffer[i] = 0;
		// Add "center text & quit early" here
	} else if (text_position == (text_queuedrender_len + TEXT_GAP)) {
		text_position = 0;
		if (text_queuedrender)
			free(text_queuedrender);
		text_queuedrender = 0;
		text_queuedrender_len = 0;
		return 1;
	}
	// advance buffer
	for (i = 0; i < ((MATRIX_X * MATRIX_Y) - 1); i++)
		text_buffer[i] = text_buffer[i + 1];

	int x;
	int y;
	for (y = 0; y < MATRIX_Y; y++) {
		// setup rightmost pixel
		text_buffer[(y * MATRIX_X) + (MATRIX_X - 1)] = text_point(y);
		for (x = 0; x < MATRIX_X; x++) {
			int v = text_buffer[x + (y * MATRIX_X)] ? 255 : 0;
			RGB color = { .red = v, .green = v, .blue = v};
			matrix_set(x, y, &color);
		}
	}
	matrix_render();
	text_position++;
	text_nexttick += TEXT_FRAMETIME;
	timer_add(text_nexttick, text_moduleno, 0, NULL);
	return 0;
}

int plugin_deinit() {
	if (text_queuedrender)
		free(text_queuedrender);
	return 0;
}
