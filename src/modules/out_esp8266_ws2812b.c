// ESP8266 i2s-based ws2812b output.
// Stolen from https://github.com/SuperHouse/esp-open-rtos/blob/master/examples/ws2812_i2s/ws2812_i2s_colour_loop.c

// Matrix order and size
#if !defined(MATRIX_ORDER_PLAIN) && !defined(MATRIX_ORDER_SNAKE)
#define MATRIX_ORDER_SNAKE // cause that's what i have, it's also the easiest to wire, IMO.
#endif

#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif

#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

#include <types.h>
#include <timers.h>
#include <matrix.h>
#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp/uart.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "ws2812_i2s/ws2812_i2s.h"

ws2812_pixel_t pixels[MATRIX_PIXELS];

int init(void) {
	ws2812_i2s_init(MATRIX_PIXELS, PIXEL_RGB);
	memset(pixels, 0, sizeof(ws2812_pixel_t) * MATRIX_PIXELS);

	return 0;
}


int getx(void) {
	return MATRIX_X;
}
int gety(void) {
	return MATRIX_Y;
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
	// No OOB check, because performance.
	ws2812_pixel_t* px = pixels[ppos(x, y)];
	px->red = color->red;
	px->green = color->green;
	px->blue = color->blue;
	return 0;
}


int clear(void) {
	memset(pixels, 0, sizeof(ws2812_pixel_t) * led_number);
	return 0;
}

int render(void) {
	ws2812_i2s_update(pixels, PIXEL_RGB);
	return 0;
}

ulong wait_until(ulong desired_usec) {
	return wait_until_core(desired_usec);
}

void wait_until_break(void) {
	return wait_until_break_core();
}

int deinit(void) {
	// No deinit method? Fine by me.
	return 0;
}
