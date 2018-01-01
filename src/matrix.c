// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include <types.h>
#include <string.h>
#include <assert.h>
#include <modloader.h>
#include <dlfcn.h>

module* outmod;

int matrix_init(int outmodno) {
	outmod = modules_get(outmodno);
	return outmod->init(outmodno);
}


byte matrix_getx(void) {
	return outmod->out_getx();
}
byte matrix_gety(void) {
	return outmod->out_gety();
}

int matrix_set(byte x, byte y, RGB *color) {
	return outmod->out_set(x, y, color);
}

// Fills part of the matrix with jo-- a single color.
int matrix_fill(byte start_x, byte start_y, byte end_x, byte end_y, RGB *color) {
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
	return outmod->out_render();
}

int matrix_deinit(void) {
	int ret = outmod->deinit();
	dlclose(outmod->lib);
	return ret;
}
