// SDL2 output plugin.

#include <types.h>
#include <string.h>
#include <assert.h>
#include <timers.h>

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

int init(void) {
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

int matrix_ppos(int x, int y) {
	return (x + (y * MATRIX_X));
}

int set(int x, int y, RGB *color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < getx());
	assert(y < gety());

	int pos = matrix_ppos(x, y) * 3;
	memcpy(&BUFFER[pos], color, sizeof(RGB));
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
		ulong tnow = utime();
		if (tnow >= desired_usec)
			return tnow;

		int sleeptimems = (desired_usec - tnow) / 1000;
		if (SDL_WaitEventTimeout(&ev, sleeptimems)) {
			if (ev.type == SDL_QUIT) {
				timers_doquit();
				return utime();
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
