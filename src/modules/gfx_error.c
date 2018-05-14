// Draw a box like this:
// +---------+
// |       |X|
// |---------+
// | no werk |
// +---------+

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <graphics.h>
#include <stdlib.h>
#include <random.h>

#include "text.h"

#define NUMTEXT 8

static char* texts[NUMTEXT] = {
	"no werk",
	"dafuq?",
	"ok wtf",
	"wat.",
	"idk.",
	"huh?",
	"osnap",
	"damn",
};

static text* rendered[NUMTEXT];

#define TOPCOL_HEIGHT 8
#define XWIDTH 8

static const int height = (TOPCOL_HEIGHT + 7 + 3); // top window decor + text width (7) + pixel spacing and plus border
static int width;

int init(int moduleno, char* argstr) {
	int maxlen = 0;
	// Render all the text snippets, get the maximum length.
	int i;
	for (i = 0; i < NUMTEXT; i++) {
		text* errtext = text_render(texts[i]);
		if (!errtext)
			return 2;

		if (errtext->len > maxlen)
			maxlen = errtext->len;

		rendered[i] = errtext;
	}

	int maxwidth = (maxlen + 4); // text size + pixel spacing and border

	if (matrix_getx() < (maxwidth + 4))
		return 1; // not enough X to be looking good
	if (matrix_gety() < (height + 4))
		return 1; // not enough Y to be looking good

	return 0;
}

static RGB bgcol = RGB(166, 166, 166); // the background, grey
static RGB bordercol = RGB(0, 0, 200); // border, blue
static RGB xcol = RGB(255, 0, 0); // red X
static RGB textcol = RGB(255, 255, 255); // white text

void draw_error(int x, int y, text* errtext) {
	// Set areas.
	int yw;
	int xw;
	int xstart = width - TOPCOL_HEIGHT;
	for (yw = 1; yw < TOPCOL_HEIGHT; yw++)
		for (xw = 1; xw < width; xw++)
			matrix_set(x + xw, y + yw, (xw <= xstart) ? &bordercol : &bgcol);

	for (yw = (TOPCOL_HEIGHT + 1); yw < height; yw++)
		for (xw = 1; xw < width; xw++)
			matrix_set(x + xw, y + yw, &bgcol);

	// Draw box:
	graphics_drawline(x, y, x + width, y, &bordercol); // top
	graphics_drawline(x, y, x, y + height, &bordercol); // left side
	graphics_drawline(x, y + TOPCOL_HEIGHT, x + width, y + TOPCOL_HEIGHT, &bordercol); // seperator
	graphics_drawline(x + width, y, x + width, y + height, &bordercol); // right side
	graphics_drawline(x, y + height, x + width, y + height, &bordercol); // bottom

	// Draw X
	int xstart_x = x + width - 8;
	graphics_drawline(xstart_x + 2, y + 2, xstart_x + 6, y + 6, &xcol);
	graphics_drawline(xstart_x + 2, y + 6, xstart_x + 6, y + 2, &xcol);

	int tbx = x + 2;
	int tby = y + TOPCOL_HEIGHT + 1;
	int tx;
	int ty;
	for (ty = 0; ty < (height - TOPCOL_HEIGHT - 2); ty++) {
		for (tx = 0; tx < (width - 2); tx++) {
			RGB col = text_point(errtext, tx, ty) ? textcol : bgcol;
			matrix_set(tbx + tx, tby + ty, &col);
		}
	}
}

int draw(int argc, char* argv[]) {
	// Pick random error message.
	text* errtext = rendered[(int) randn((unsigned int) (NUMTEXT - 1))];
	width = (errtext->len + 4); // text size + pixel spacing and border

	// draw single error at center, for now.
	draw_error((matrix_getx() / 2) - 1 - (width / 2), (matrix_gety() / 2) - 1 - (height / 2), errtext);
	matrix_render();
	return 0;
}

void reset(void) {
	// Nothing?
}

int deinit(void) {
	// This acts conditionally on rendered being non-NULL.
	int i;
	for (i = 0; i < NUMTEXT; i++) {
		text_free(rendered[i]);
	}
	return 0;
}
