// Matrix order and size
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

#include "../ext/card10/epicardium.h"

#include "FreeRTOS.h"
#include <types.h>
#include <timers.h>
#include <matrix.h>
#include "timers.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <colors.h>

static int counter = 0;
union disp_framebuffer fb;

int init(void) {
	return epic_disp_open();
}

int deinit(void) {
	return epic_disp_close();
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
	//size_t px = ppos(x, y);
	// TODO
	return RGB(0,0,0);
}

// TODO flix axis
int set(int modno, int x, int y, RGB color) {
	// No OOB check, because performance.
	uint16_t converted = RGB2RGB565(color);
	fb.fb[MATRIX_Y-1-y][x][0] = converted >> 8;
	fb.fb[MATRIX_Y-1-y][x][1] = converted & 0xFF;
	return 0;
}


int clear(void) {
	for (int y = 0; y < MATRIX_Y; y++) {
		for (int x = 0; x < MATRIX_X; x++) {
			fb.fb[y][x][0] = 0;
			fb.fb[y][x][1] = 0;
		}
	}
	return 0;
}

int render(void) {
	return epic_disp_framebuffer(&fb);
}

ulong wait_until(ulong desired_usec) {
	return;
	//return timers_wait_until_core(desired_usec);
}

void wait_until_break(void) {
	return;
	//timers_wait_until_break_core();
}

