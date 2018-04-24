// SDL2 output plugin.

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


#include <types.h>
#include <string.h>
#include <assert.h>
#include <timers.h>
#include <modloader.h>

// Calculation for amount of bytes needed.
#define ROW_SIZE MATRIX_X * 3 // 3 for R, G and B.
#define BUFFER_SIZE ROW_SIZE * MATRIX_Y
#define MATRIX_PIXELS (MATRIX_X * MATRIX_Y)

#include <SDL2/SDL.h>

// SDL-based stuff, we need to create a buffer.
static byte BUFFER[BUFFER_SIZE];

#define WIN_W (MATRIX_X * SDL_SCALE_FACTOR)
#define WIN_H (MATRIX_Y * SDL_SCALE_FACTOR)

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Rect dest = { .x = 0, .y = 0, .w = WIN_W, .h = WIN_H };

module *this = NULL;

int init(int modno, char *argstr) {
	this = modules_get(modno);

	if (SDL_Init(SDL_INIT_VIDEO))
		return 2;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	window = SDL_CreateWindow("sled: DEBUG Platform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, MATRIX_X, MATRIX_Y);

	memset(BUFFER, 0, BUFFER_SIZE);

	return 0;
}


int getx(void) {
	return MATRIX_X; // for now.
}
int gety(void) {
	return MATRIX_Y; // for now.
}

static int matrix_ppos(int x, int y) {
	return (x + (y * MATRIX_X));
}

int set(int x, int y, RGB *color) {
	if (x < 0 || y < 0)
		return 1;
	if (x >= this->getx() || y >= this->gety())
		return 2;

	int pos = matrix_ppos(x, y) * 3;
	BUFFER[pos + 0] = color->red;
	BUFFER[pos + 1] = color->green;
	BUFFER[pos + 2] = color->blue;
	return 0;
}

// Zeroes the stuff.
int clear(void) {
	memset(&BUFFER, 0, BUFFER_SIZE);
	return 0;
}

int render(void) {
	SDL_UpdateTexture(texture, NULL, BUFFER, ROW_SIZE);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, &dest);
	SDL_RenderPresent(renderer);
	return 0;
}

ulong wait_until(ulong desired_usec) {
	SDL_Event ev;
	while (1) {
		ulong tnow = udate();
		if (tnow >= desired_usec)
			return tnow;

		int sleeptimems = (desired_usec - tnow) / 1000;
		if (SDL_WaitEventTimeout(&ev, sleeptimems)) {
			if (ev.type == SDL_QUIT) {
				timers_doquit();
				return udate();
			}
		} else {
			return wait_until_core(desired_usec);
		}
	}
}

int deinit(void) {
	// Destroy everything.
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
