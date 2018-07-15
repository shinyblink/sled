// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include "types.h"
#include <string.h>
#include <assert.h>
#include "mod.h"
#include "loadcore.h"
#include "main.h"
#include <stdlib.h>

// Filters. Index 0 is the surface layer of the API, unless there are no filters, in which case that responsibility falls to the output module.
// Everything in here, and the output module, is inited and deinited manually.
// A filter is obligated to deinit the module below it in the chain (but not init it), so only the surface-layer module must be deinited from here.
// The last index is directly above the true output module itself.
static int* filters;
static int filter_amount = 0;

// Not to be confused with outmod, this is the contents of filters[0] if it exists, otherwise the output module structure.
static mod_out *out;

int matrix_init(int outmodno, int* filter_list, int filtno, char* outarg, char** filtargs) {
	filters = filter_list;
	filter_amount = filtno;

	int ret = outmod->init(outmodno, outarg);
	if (ret != 0) return ret;

	if (filtno > 0) {
		int i = filtno;
		int last = outmodno;
		for (i = (filtno - 1); i >= 0; --i) {
			ret = mod_get(filters[i])->init(last, filtargs[i]);
			if (ret != 0) return ret;
			last = filters[i];
		}
		outmod = mod_get(filters[0]);
	}
	out = outmod->mod;
	return ret;
}

int matrix_getx(void) {
	return out->getx();
}
int matrix_gety(void) {
	return out->gety();
}

int matrix_set(int x, int y, RGB color) {
	return out->set(x, y, color);
}

RGB matrix_get(int x, int y) {
	return out->get(x, y);
}

// Fills part of the matrix with jo-- a single color.
int matrix_fill(int start_x, int start_y, int end_x, int end_y, RGB color) {
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
	return out->clear();
}

int matrix_render(void) {
	return out->render();
}

int matrix_deinit(void) {
	int ret = 0;
	if (outmod != NULL)
		ret = outmod->deinit(mod_getid(outmod));
	return ret;
}
