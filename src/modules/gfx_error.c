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

#include "text.h"

#define TEXT "no werk"
#define TOPCOL_HEIGHT 8
#define XWIDTH 8

static int width;
static int height;

static text* errtext = NULL;

int init(int moduleno, char* argstr) {
	errtext = text_render(TEXT);
	if (!errtext)
		return 2;

	width = (errtext->len + 4); // text size + pixel spacing and border
	height = (TOPCOL_HEIGHT + 7 + 3); // top window decor + text width (7) + pixel spacing and plus border


	if (matrix_getx() < (width + 4))
		return 1; // not enough X to be looking good
	if (matrix_gety() < (height + 4))
		return 1; // not enough Y to be looking good

	return 0;
}

static RGB bgcol = RGB(166, 166, 166); // the background, grey
static RGB bordercol = RGB(0, 0, 200); // border, blue
static RGB xcol = RGB(255, 0, 0); // red X
static RGB textcol = RGB(255, 255, 255); // white text

void draw_error(int x, int y) {
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
	// draw single error at center, for now.
	draw_error((matrix_getx() / 2) - 1 - (width / 2), (matrix_gety() / 2) - 1 - (height / 2));
	matrix_render();
	return 0;
}

int deinit() {
	// This acts conditionally on rendered being non-NULL.
	text_free(errtext);
	return 0;
}
