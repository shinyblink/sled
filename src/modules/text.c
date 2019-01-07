// Small proportional text renderer.
// Font is 8px tall at maximum.

#include <types.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "font.h"
#include "text.h"

byte text_point(text* rendered, int x, int y) {
	if (x < 0)
		return 0;
	if (y < 0)
		return 0;
	if (y > 7)
		return 0;
	if (rendered == NULL)
		return 0;
	if (x >= rendered->len)
		return 0;
	if (!rendered->buffer)
		return 0;
	return rendered->buffer[x].data[y];
}

static void text_putpixel(void * ud, int x, int y, int c) {
	text_column * tc = (text_column *) ud;
	tc[x].data[y] = c;
}

static int text_render_core(const char* txt, text_column* outp) {
	// Same "dud run" approach as before.
	int columns = 0;
	int first = 1;
	while (*txt) {
		if (outp) {
			int co = font_draw((byte) ((*txt) - 0x20), outp, text_putpixel);
			columns += co;
			outp += co;
		} else {
			columns += font_draw((byte) ((*txt) - 0x20), NULL, NULL);
		}
		first = 0;
		txt++;
	}
	if (!first)
		columns--;
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
		rendered->buffer = malloc(rendered->len * sizeof(text_column));
		if (!rendered->buffer) {
			free(rendered);
			return NULL;
		}
		// Zero the whole thing.
		memset(rendered->buffer, 0, rendered->len * sizeof(text_column));
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
