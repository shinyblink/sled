#include "display.h"
#include "Fonts/fonts.h"
#include "FreeRTOS.h"
#include "LCD_Driver.h"
#include "epicardium.h"
#include "gfx.h"
#include "gpio.h"
#include "task.h"
#include "tmr.h"
#include "tmr_utils.h"

static TaskHandle_t lock = NULL;

static int check_lock()
{
	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	if (task != lock) {
		return -EBUSY;
	} else {
		return 0;
	}
}

int epic_disp_print(
	uint16_t posx,
	uint16_t posy,
	const char *pString,
	uint16_t fg,
	uint16_t bg
) {
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	} else {
		gfx_puts(&Font20, &display_screen, posx, posy, pString, fg, bg);
		return 0;
	}
}

int epic_disp_clear(uint16_t color)
{
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	} else {
		gfx_clear_to_color(&display_screen, color);
		return 0;
	}
}

int epic_disp_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	} else {
		gfx_setpixel(&display_screen, x, y, color);
		return 0;
	}
}

int epic_disp_line(
	uint16_t xstart,
	uint16_t ystart,
	uint16_t xend,
	uint16_t yend,
	uint16_t color,
	enum disp_linestyle linestyle,
	uint16_t pixelsize
) {
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	} else {
		/* TODO add linestyle support to gfx code */
		gfx_line(
			&display_screen,
			xstart,
			ystart,
			xend,
			yend,
			pixelsize,
			color
		);
		return 0;
	}
}

int epic_disp_rect(
	uint16_t xstart,
	uint16_t ystart,
	uint16_t xend,
	uint16_t yend,
	uint16_t color,
	enum disp_fillstyle fillstyle,
	uint16_t pixelsize
) {
	int cl = check_lock();
	if (cl < 0)
		return cl;

	switch (fillstyle) {
	case FILLSTYLE_EMPTY:
		gfx_rectangle(
			&display_screen,
			xstart,
			ystart,
			xend - xstart,
			yend - ystart,
			pixelsize,
			color
		);
		break;
	case FILLSTYLE_FILLED:
		gfx_rectangle_fill(
			&display_screen,
			xstart,
			ystart,
			xend - xstart,
			yend - ystart,
			color
		);
		break;
	}
	return 0;
}

int epic_disp_circ(
	uint16_t x,
	uint16_t y,
	uint16_t rad,
	uint16_t color,
	enum disp_fillstyle fillstyle,
	uint16_t pixelsize
) {
	int cl = check_lock();
	if (cl < 0)
		return cl;

	switch (fillstyle) {
	case FILLSTYLE_EMPTY:
		gfx_circle(&display_screen, x, y, rad, pixelsize, color);
		break;
	case FILLSTYLE_FILLED:
		gfx_circle_fill(&display_screen, x, y, rad, color);
		break;
	}

	return 0;
}

int epic_disp_update()
{
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	}

	gfx_update(&display_screen);
	return 0;
}

int epic_disp_framebuffer(union disp_framebuffer *fb)
{
	int cl = check_lock();
	if (cl < 0) {
		return cl;
	}

	LCD_Set(fb->raw, sizeof(fb->raw));
	return 0;
}

int epic_disp_open()
{
	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	if (lock == task) {
		return 0;
	} else if (lock == NULL) {
		lock = task;
		return 0;
	} else {
		return -EBUSY;
	}
}

int epic_disp_close()
{
	if (check_lock() < 0 && lock != NULL) {
		return -EBUSY;
	} else {
		lock = NULL;
		return 0;
	}
}

void disp_forcelock()
{
	TaskHandle_t task = xTaskGetCurrentTaskHandle();
	lock              = task;
}
