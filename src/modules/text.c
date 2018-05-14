// Small proportional text renderer.
// Font is 8px tall at maximum.

#include <types.h>
#include <stdlib.h>
#include <assert.h>

#include "font.h"

typedef struct text {
	int len;
	byte* buffer;
} text;

int text_point(text* rendered, int x, int y) {
	if (x < 0)
		return 0;
	if (y < 0)
		return 0;
	if (y > 7)
		return 0;
	if (x >= rendered->len)
		return 0;
	if (!rendered)
		return 0;
	if (!rendered->buffer)
		return 0;
	return rendered->buffer[x] & (1 << y);
}

static int text_render_core(const char* txt, byte* outp) {
	int columns = 0;
	int first = 1;
	while (*txt) {
		int space = 0;
		if (!first)
			space = 1;
		first = 0;
		byte* fontptr = font_data;
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

text* text_render(const char * txt) {
	text* rendered = malloc(sizeof(text));
	if (!rendered)
		return NULL;
	rendered->len = text_render_core(txt, NULL);
	rendered->buffer = NULL;
	// pretty sure malloc(0) is undefined or something, don't do it
	if (rendered->len != 0) {
		rendered->buffer = malloc(rendered->len);
		if (!rendered->buffer) {
			free(rendered);
			return NULL;
		}
		text_render_core(txt, rendered->buffer);
	}
	return rendered;
}

void text_free(text* rendered) {
	if (rendered) {
		if (rendered->buffer)
			free(rendered->buffer);
		free(rendered);
	}
}
