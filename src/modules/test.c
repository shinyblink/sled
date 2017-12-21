// Small test module.

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <stdio.h>

int plugin_init() {																	
	printf("Hello from test plugin!\n");
	return 0;
}

int plugin_draw() {
	RGB color = { .red = 255, .green = 0, .blue = 0};

	int x;
	int y;
	for (y = 0; y < MATRIX_Y; y++) 
		for (x = 0; x < MATRIX_X; x++)
			matrix_set(x, y, &color);

	matrix_render();
	return 0;
}

int plugin_deinit() {
	printf("Goodbye, oh testy world.\n");
	return 0;
}
