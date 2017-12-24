// The things actually doing the matrix manipulation.
// Also contains the buffers.

#include <types.h>
#include <string.h>

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
#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

#ifdef PLATFORM_SDL2
#include <SDL2/SDL.h>

// SDL-based stuff, we need to create a buffer.
static byte BUFFER[BUFFER_SIZE];

#define WIN_W (MATRIX_X * SDL_SCALE_FACTOR)
#define WIN_H (MATRIX_Y * SDL_SCALE_FACTOR)

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Rect dest = { .x = 0, .y = 0, .w = WIN_W, .h = WIN_H };

#elif defined(PLATFORM_RPI)
// TODO: include the many headers, init the struct it wants.

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
			.strip_type = WS2811_STRIP_GBR,
		},
		[1] = {
			.gpionum = 0,
			.count = 0,
			.invert = 0,
			.brightness = 0
		}
	}
};
#endif

int matrix_init(void) {
	#ifdef PLATFORM_SDL2
	if (SDL_Init(SDL_INIT_VIDEO))
		return 2;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	window = SDL_CreateWindow("sled: DEBUG Platform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, MATRIX_X, MATRIX_Y);

	memset(BUFFER, 0, BUFFER_SIZE);
	#elif defined(PLATFORM_RPI)
	ws2811_return_t ret;
	if ((ret = ws2811_init(&leds)) != WS2811_SUCCESS) {
		eprintf("matrix: ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
		return 2;
	}
	#endif
	return 0;
}

int matrix_set(byte x, byte y, RGB *color) {
	if (x > MATRIX_X)
		return 1;
	if (y > MATRIX_Y)
		return 2;

	#ifdef PLATFORM_SDL2
	int pos = PIXEL_POS(x, y) * 3;
	memcpy(&BUFFER[pos], color, sizeof(RGB));
	#elif PLATFORM_RPI
	ws2811_led_t led = (color->red << 16) | (color->green << 8) | color->blue;
	leds.channel[0].leds[PIXEL_POS(x, y)] = led;
	#endif
	return 0;
}

// Fills part of the matrix with jo-- a single color.
int matrix_fill(byte start_x, byte start_y, byte end_x, byte end_y, RGB *color) {
	if (end_x > MATRIX_X)
		return 1;
	if (end_y > MATRIX_Y)
		return 2;

	if (start_x > end_x)
		return 3;
	if (start_y > end_y)
		return 4;

	int ret = 0;
	int x;
	int y;
	#ifdef PLATFORM_SDL2
	for (y=start_y; y <= end_y; y++)
		for (x=start_x; x <= end_x; x++)
			memcpy(&BUFFER[PIXEL_POS(x, y) * 3], color, sizeof(RGB));
	#else
	for (y=start_y; y <= end_y; y++)
		for (x=start_x; x <= end_x; x++) {
			ret = matrix_set(x, y, color);
			if (ret != 0)
				return ret;
		}
	#endif
	return ret;
}

// Zeroes the stuff.
int matrix_clear(void) {
	#ifdef PLATFORM_SDL2
	memset(&BUFFER, 0, BUFFER_SIZE);
	//#elif PLATFORM_RPI
	//memset(&leds.channel[0].leds, 0, MATRIX_PIXELS * sizeof(uint32_t));
	#else
	RGB color = { .red = 0, .green = 0, .blue = 0 };
	matrix_fill(0, 0, MATRIX_X - 1, MATRIX_Y - 1, &color);
#endif
	return 0;
}

int matrix_render(void) {
	#ifdef PLATFORM_SDL2
	SDL_UpdateTexture(texture, NULL, BUFFER, ROW_SIZE);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, &dest);
	SDL_RenderPresent(renderer);
	#elif defined(PLATFORM_RPI)
	ws2811_return_t ret;
	if ((ret = ws2811_render(&leds)) != WS2811_SUCCESS) {
		eprintf("matrix: ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
	}
	#endif
	return 0;
}

int matrix_deinit(void) {
	#ifdef PLATFORM_SDL2
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	#elif defined(PLATFORM_RPI)
	ws2811_fini(&leds);
	#endif
	return 0;
}
