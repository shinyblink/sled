// Matrix order and size
#define LCD_X 160
#define LCD_Y 80

#define LCD_PIXELS (LCD_X * LCD_Y)

#include "../ext/card10/epicardium.h"

#include "FreeRTOS.h"
#include <types.h>
#include <timers.h>
#include <matrix.h>
#include <timers.h>
#include <colors.h>
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
	return LCD_X;
}
int gety(void) {
	return LCD_Y;
}

int ppos(int x, int y) {
	return (x + (y * LCD_X));
}

RGB get(int _modno, int x, int y) {
	uint16_t converted = 0;
	converted = fb.fb[LCD_Y-1-y][x][0] << 8;
	converted += fb.fb[LCD_Y-1-y][x][1];
	return RGB5652RGB(converted);
}

int set(int modno, int x, int y, RGB color) {
	// No OOB check, because performance.
	uint16_t converted = RGB2RGB565(color);
	fb.fb[LCD_Y-1-y][x][0] = converted >> 8;
	fb.fb[LCD_Y-1-y][x][1] = converted & 0xFF;
	return 0;
}


int clear(void) {
	for (int y = 0; y < LCD_Y; y++) {
		for (int x = 0; x < LCD_X; x++) {
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
	return 0;
	//return timers_wait_until_core(desired_usec);
}

void wait_until_break(void) {
	return;
	//timers_wait_until_break_core();
}
