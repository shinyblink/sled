// Small module that randomly sets a static color.

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <stdio.h>
#include <random.h>

int plugin_init(int moduleno) {
	return 0;
}

int plugin_draw() {
	RGB color = { .red = randn(255), .green = randn(255), .blue = randn(255) };

	int x;
	int y;
	for (y = 0; y < MATRIX_Y; y++) 
		for (x = 0; x < MATRIX_X; x++)
			matrix_set(x, y, &color);

	matrix_render();
	return 0;
}

int plugin_deinit() {
	return 0;
}
