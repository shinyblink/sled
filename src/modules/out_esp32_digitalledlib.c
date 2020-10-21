// Matrix order and size
#define MATRIX_ORDER_SNAKE
#define GPIO_PIN 4

#if !defined(MATRIX_ORDER_PLAIN) && !defined(MATRIX_ORDER_SNAKE)
#define MATRIX_ORDER_PLAIN
#endif

#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif

#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

#include <freertos/FreeRTOS.h>
#include <types.h>
#include <timers.h>
#include <matrix.h>
#include "timers.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../ext/esp32/esp32_digital_led_lib.h"
static strand_t pStrand;

int init(void) {
	pStrand.rmtChannel = 0;
	pStrand.gpioNum = GPIO_PIN;
	pStrand.ledType = LED_WS2812B_V3;
	pStrand.brightLimit = 255;
	pStrand.numPixels = MATRIX_X * MATRIX_Y;
	pStrand.pixels = NULL;
	pStrand._stateVars = NULL;

	if (digitalLeds_initStrands(&pStrand, 1)) {
		printf("led init fail");
		return 1;
    }
	digitalLeds_resetPixels(&pStrand);

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

RGB get(int _modno, int x, int y) {
	size_t px = ppos(x, y);
	return RGB(pStrand.pixels[px].r, pStrand.pixels[px].g, pStrand.pixels[px].b);
}

int set(int modno, int x, int y, RGB color) {
	// No OOB check, because performance.
	size_t px = ppos(x, y);
	pStrand.pixels[px].r = color.red;
	pStrand.pixels[px].g = color.green;
	pStrand.pixels[px].b = color.blue;
	return 0;
}


int clear(void) {
	digitalLeds_resetPixels(&pStrand);
	return 0;
}

int render(void) {
	digitalLeds_updatePixels(&pStrand);
	return 0;
}

oscore_time wait_until(int moduleno, oscore_time desired_usec) {
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int moduleno) {
	timers_wait_until_break_core();
}

int deinit(void) {
	// No deinit method? Fine by me.
	return 0;
}
