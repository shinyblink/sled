// output module for the nSpire CX with ndless
// does not support the flipped LCD on Rev W and later yet

// -- std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// -- sled
#include "../colors.h"
#include "../timers.h"
#include "../types.h"
// -- ndls
#include <libndls.h>

#define LCD_X 320
#define LCD_Y 240

#ifndef SCALE_FACTOR
#define SCALE_FACTOR 2
#endif

static uint16_t* frameBuffer;
static scr_type_t screen_type;

int init(int moduleno, char * argstr) {
	frameBuffer = malloc(LCD_X * LCD_Y * sizeof(uint16_t));
	if (!frameBuffer) {
		show_msgbox("sled", "Couldn't allocate frame buffer");
		return 1;
	}

	// LCD sanity check.
	screen_type = lcd_type();
	if (screen_type != SCR_320x240_565) {
		free(frameBuffer);
		show_msgbox("sled", "Can't run on classic nSpire calculators.");
		return 1;
	}
	if (!lcd_init(SCR_320x240_565)) {
		show_msgbox("sled", "Failed to initialize screen.");
		free(frameBuffer);
		return 1;
	}

	if (screen_type != SCR_320x240_565) {
		show_msgbox("sled", "Scaling to non-native LCD.");
	}

	return 0;
}

int getx(int _modno) {
	return LCD_X / SCALE_FACTOR;
}
int gety(int _modno) {
	return LCD_Y / SCALE_FACTOR;
}


#define PPOS(x, y) ((x) + ((y) * LCD_X))

int set(int _modno, int x, int y, RGB color) {
	// no bounds check for performance, yolo
	uint16_t rgb565 = RGB2RGB565(color);

	for (int yo = 0; yo < SCALE_FACTOR; yo++)
		for (int xo = 0; xo < SCALE_FACTOR; xo++)
			frameBuffer[PPOS((x * SCALE_FACTOR) + xo, (y * SCALE_FACTOR) + yo)] = rgb565;
	return 0;
}

RGB get(int _modno, int x, int y) {
	// no bounds check, yolo
	uint16_t rgb565 = frameBuffer[PPOS(x * SCALE_FACTOR, y * SCALE_FACTOR)];
	return RGB5652RGB(rgb565);
}

int clear(__unused int _modno) {
	memset(frameBuffer, 0, LCD_X * LCD_Y * sizeof(uint16_t));
	return 0;
}

int render(int moduleno) {
	if (any_key_pressed()) {
		// check for {"esc", "menu", "home"} keys to exit
		if (isKeyPressed(KEY_NSPIRE_ESC) || isKeyPressed(KEY_NSPIRE_MENU) || isKeyPressed(KEY_NSPIRE_HOME))
			timers_doquit();
	}

	lcd_blit(frameBuffer, screen_type);
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	free(frameBuffer);
	lcd_init(SCR_TYPE_INVALID); // Reset LCD.
}
