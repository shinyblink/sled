// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include "types.h"
#include <string.h>
#include <assert.h>
#include "modloader.h"
#include "loadcore.h"

static module* outmod;

int* filters;
int filter_amount = 0;

int matrix_init(int outmodno, int* filter_list, int filtno, char* outarg, char** filtargs) {
	filters = filter_list;
	filter_amount = filtno;

	outmod = modules_get(outmodno);
	int ret = outmod->init(outmodno, outarg);
	if (ret != 0) return ret;

	if (filtno > 0) {
		int i = filtno;
		int last = outmodno;
		for (i = (filtno - 1); i >= 0; --i) {
			ret = modules_get(filters[i])->init(last, filtargs[i]);
			if (ret != 0) return ret;
			last = filters[i];
		};
		outmod = modules_get(filters[0]);
	};
	return ret;
}

int matrix_getx(void) {
	return outmod->getx();
}
int matrix_gety(void) {
	return outmod->gety();
}

int matrix_set(int x, int y, const RGB *color) {
	return outmod->set(x, y, color);
}

// Fills part of the matrix with jo-- a single color.
int matrix_fill(int start_x, int start_y, int end_x, int end_y, RGB *color) {
	if (start_x > end_x)
		return 1;
	if (start_y > end_y)
		return 2;

	int x;
	int y;

	for (y = MAX(start_y, 0); y <= MIN(end_y, matrix_gety()); y++)
		for (x = MAX(start_x, 0); x <= MIN(end_x, matrix_getx()); x++) {
			matrix_set(x, y, color);
		}
	return 0;
}

// Zeroes the stuff.
int matrix_clear(void) {
	return outmod->clear();
}

int matrix_render(void) {
	return outmod->render();
}

int matrix_deinit(void) {
	int ret = 0;
	if (outmod != NULL) {
		ret = outmod->deinit();
		loadcore_close(outmod->lib);
	}
	return ret;
}
