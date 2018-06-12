// Small module that randomly sets a static color.

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <stdio.h>
#include <random.h>

int init(int moduleno, char* argstr) {
	return 0;
}

int draw(int argc, char* argv[]) {
	RGB color = RGB(randn(220), randn(220), randn(220));

	matrix_fill(0, 0, matrix_getx() - 1, matrix_gety() - 1, color);

	matrix_render();
	return 0;
}

void reset(void) {
	// Nothing?
}

int deinit(void) {
	return 0;
}
