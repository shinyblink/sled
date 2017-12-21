// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include <types.h>

#ifdef MATRIX_ORDER_PLAIN
#define PIXEL_POS(x, y) (x + y*MATRIX_X)
#elif defined(MATRIX_ORDER_SNAKE)
// Order is like this
// 0 1 2
// 5 4 3
// 6 7 8
#define PIXEL_POS(x, y) (((y % 2) == 0 ? x : (MATRIX_X - 1) - x) + MATRIX_X*y)
#endif

// Calculation for amount of bytes needed.

#define ROW_SIZE MATRIX_X * 3 // 3 for R, G and B.
#define BUFFER_SIZE ROW_SIZE * MATRIX_Y

#ifdef PLATFORM_DEBUG
// SDL-based stuff, we need to create a buffer.
byte BUFFER[BUFFER_SIZE];

#elif defined(PLATFORM_RPI)
// TODO: include the many headers, init the struct it wants.
#endif

int matrix_init() {
	#ifdef PLATFORM_DEBUG
	// TODO: Init SDL here.
	#elif defined(PLATFORM_RPI)
	// TODO: init the ws281X library here
	#endif
	return 0;
}

int matrix_set(byte x, byte y, RGB *color) {
	#ifdef PLATFORM_DEBUG
	memcpy(&BUFFER[PIXEL_POS(x, y) * 3], &color, sizeof RGB);
	#elif PLATFORM_RPI
	// TODO: write into the DMA framebuffer from the library, similar to the above.
	#endif
	return 0;
}

int matrix_render() {
	#ifdef PLATFORM_DEBUG
	// TODO: Redraw SDL.
	#elif defined(PLATFORM_RPI)
	// TODO: call ws2811_render()
	#endif
	return 0;
}

int matrix_deinit() {
	#ifdef PLATFORM_DEBUG
	// TODO: Destroy SDL window, cleanup.
	#elif defined(PLATFORM_RPI)
	// TODO: call ws2811_fini()
	#endif
	return 0;
}
