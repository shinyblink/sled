// Small module that randomly sets a static color.

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <stdio.h>
#include <random.h>

int plugin_init(int moduleno) {
	return 0;
}

int plugin_draw(int argc, char* argv[]) {
	RGB color = { .red = randn(255), .green = randn(255), .blue = randn(255) };

	matrix_fill(0, 0, MATRIX_X - 1, MATRIX_Y - 1, &color);

	matrix_render();
	return 0;
}

int plugin_deinit(void) {
	return 0;
}
