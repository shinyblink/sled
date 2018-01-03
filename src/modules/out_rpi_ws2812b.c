// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include <types.h>
#include <string.h>
#include <assert.h>
#include <timers.h>
#include <matrix.h>

// Calculation for amount of bytes needed.

#define ROW_SIZE MATRIX_X * 3 // 3 for R, G and B.
#define BUFFER_SIZE ROW_SIZE * MATRIX_Y
#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

#include <stdint.h>
#include <stdio.h>

#include <clk.h>
#include <gpio.h>
#include <dma.h>
#include <pwm.h>
#include <ws2811.h>

ws2811_t leds = {
	.freq = WS2811_TARGET_FREQ,
	.dmanum = RPI_DMA,
	.channel = {
		[0] = {
			.gpionum = RPI_PIN,
			.count = MATRIX_PIXELS,
			.invert = 0,
			.brightness = 255,
			.strip_type = WS2811_STRIP_RGB
		},
		[1] = {
			.gpionum = 0,
			.count = 0,
			.invert = 0,
			.brightness = 0
		}
	}
};

int init(void) {
	ws2811_return_t ret;
	if ((ret = ws2811_init(&leds)) != WS2811_SUCCESS) {
		eprintf("matrix: ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
		return 2;
	}
	return 0;
}


int getx(void) {
	return MATRIX_X; // for now.
}
int gety(void) {
	return MATRIX_Y; // for now.
}

int ppos(int x, int y) {
#ifdef MATRIX_ORDER_PLAIN
	return (x + (y * MATRIX_X));
#elif defined(MATRIX_ORDER_SNAKE)
	// Order is like this
	// 0 1 2
	// 5 4 3
	// 6 7 8
	return (((y % 2) == 0 ? x : (MATRIX_X - 1) - x) + MATRIX_X*y);
#endif
}

int set(int x, int y, RGB *color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < getx());
	assert(y < gety());

#ifdef COLOR_ORDER_RGB
	ws2811_led_t led = (color->red << 16) | (color->green << 8) | color->blue;
#elif defined(COLOR_ORDER_GBR)
	ws2811_led_t led = (color->green << 16) | (color->blue << 8) | color->red;
#elif defined(COLOR_ORDER_GRB)
	ws2811_led_t led = (color->green << 16) | (color->red << 8) | color->blue;
#else
#error Must define color order.
#endif
	leds.channel[0].leds[ppos(x, y)] = led;
	return 0;
}

// Zeroes the stuff.
int clear(void) {
	RGB color = { .red = 0, .green = 0, .blue = 0 };
	matrix_fill(0, 0, MATRIX_X - 1, MATRIX_Y - 1, &color);
	return 0;
}

int render(void) {
	ws2811_return_t ret;
	if ((ret = ws2811_render(&leds)) != WS2811_SUCCESS) {
		eprintf("matrix: ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
	}
	return 0;
}

ulong wait_until(ulong desired_usec) {
	return wait_until_core(desired_usec);
}

int deinit(void) {
	ws2811_fini(&leds);
	return 0;
}
