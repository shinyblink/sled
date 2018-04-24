// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include "types.h"
#include <string.h>
#include <assert.h>
#include "modloader.h"
#include <dlfcn.h>

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

int matrix_set(int x, int y, RGB *color) {
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

	for (y=start_y; y <= end_y; y++)
		for (x=start_x; x <= end_x; x++) {
			matrix_set(x, y, color);
		}
	return 0;
}

// Zeroes the stuff.
int matrix_clear(void) {
	RGB color = { .red = 0, .green = 0, .blue = 0 };
	matrix_fill(0, 0, matrix_getx() - 1, matrix_gety() - 1, &color);
	return 0;
}

int matrix_render(void) {
	return outmod->render();
}

int matrix_deinit(void) {
	int ret = 0;
	if (outmod != NULL) {
		ret = outmod->deinit();
		dlclose(outmod->lib);
	}
	return ret;
}
