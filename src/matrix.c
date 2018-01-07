// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include <types.h>
#include <string.h>
#include <assert.h>
#include <modloader.h>
#include <dlfcn.h>

module* outmod;

module** filters;
int filter_amount = 0;

int matrix_init(int outmodno, module** filter_list, int filtno) {
	filters = filter_list;
	filter_amount = filtno;

	outmod = modules_get(outmodno);
	return outmod->init(outmodno);
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

	int ret = 0;
	int x;
	int y;

	for (y=start_y; y <= end_y; y++)
		for (x=start_x; x <= end_x; x++) {
			ret = matrix_set(x, y, color);
			if (ret != 0)
				return ret;
		}
	return ret;
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
	int ret = outmod->deinit();
	dlclose(outmod->lib);
	return ret;
}
