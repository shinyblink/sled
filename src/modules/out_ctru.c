// Actually now a libctru output

#include <types.h>
#include <timers.h>
#include <stdlib.h>
#include <3ds.h>
#include <string.h>

#define SCREEN_ID GFX_TOP
#define CONSOLE_ID GFX_BOTTOM
static u16 lcd_w, lcd_h;
static u16 fb_w, fb_h;
static u32* lcd_fb = NULL;
static u32* fb = NULL;

// Set up flags for scaled transfer
#define DISPLAY_TRANSFER_FLAGS																					\
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	 GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	 GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

int init(void) {
	gfxInitDefault();

	consoleInit(CONSOLE_ID, NULL);

	gfxSet3D(false);
	gfxSetScreenFormat(SCREEN_ID, GSP_RGBA8_OES);
	gfxSetDoubleBuffering(SCREEN_ID, false);
	gfxSwapBuffersGpu();
	gspWaitForVBlank();

	// Get our real framebuffer and alloc our own.
	// w/h is switched on purpose.
	lcd_fb = (u32*) gfxGetFramebuffer(SCREEN_ID, GFX_LEFT, &lcd_h, &lcd_w);
	fb_w = lcd_w; /// 2;
	fb_h = lcd_h; /// 2;

	fb = linearAlloc(fb_w * fb_h * 4); // RGBA8
	return 0;
}

int getx(void) {
	return fb_w;
}
int gety(void) {
	return fb_h;
}

int set(int x, int y, RGB *color) {
	// Ideally we'd want this to be here, however, since we want decent performance even on the old 3DS,
	// this check just isn't in the budget.
	/*
		if (x < 0 || y < 0)
		return 1;
		if (x >= matrix_w || y >= matrix_h)
		return 2; */
	// W/H are in reverse order and this occurs for 90-degree rotation.
	y = fb_h - (1 + y);
	fb[y + (x * fb_h)] = (color->red << 24) | (color->green << 16) | (color->blue << 8);
	return 0;
}

int clear(void) {
	memset(fb, 0, fb_w * (size_t) fb_h * (size_t) 4);
	return 0;
};

int render(void) {
	gspWaitForVBlank();

	GSPGPU_FlushDataCache(fb, fb_w * fb_h * 4);
	GX_DisplayTransfer(fb, GX_BUFFER_DIM((u32) fb_w, (u32) fb_h), lcd_fb, GX_BUFFER_DIM((u32) lcd_w, (u32) lcd_h), DISPLAY_TRANSFER_FLAGS);
	gspWaitForPPF();

	// Check if start is pressed, if it is, exit.
	hidScanInput();
	// Raise the shutdown alarm.
	if ((!aptMainLoop()) || (hidKeysDown() & KEY_START))
		timers_doquit();
	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return wait_until_core(desired_usec);
}

void wait_until_break(void) {
	wait_until_break_core();
}

int deinit(void) {
	// Can we just.. chill for a moment, please?
	gfxExit();
	return 0;
}
