// Simple screen grabber that draws the upper left corner of the screen to the display
//
// Copyright (c) 2019, X41 <whyareyoureadingmycode@x41.me>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/X.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

static int modno;
static int frame;
static ulong nexttick;

int width;
int height;

Display *display;
Window root;
XImage *image;

unsigned long red_mask;
unsigned long green_mask;
unsigned long blue_mask;
unsigned long pixel;
unsigned char blue;
unsigned char green;
unsigned char red;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;

	// grab X11 screen
	display = XOpenDisplay(NULL);
	root = DefaultRootWindow(display);
	XWindowAttributes gwa;
	XGetWindowAttributes(display, root, &gwa);

	// use dimensions of matrix
	width = matrix_getx();
	height = matrix_gety();

	// get first frame and set up color masks
	image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
	red_mask   = image->red_mask;
	green_mask = image->green_mask;
	blue_mask  = image->blue_mask;
	return 0;
}

void reset(void) {
	nexttick = udate();
}

int draw(int argc, char* argv[]) {
	nexttick = udate() + FRAMETIME;

	image = XGetImage(display,root, 0,0 , width,height,AllPlanes, ZPixmap);

	for (int x = 0; x < width; x++){
		for (int y = 0; y < height ; y++){
			pixel = XGetPixel(image,x,y);

			blue = pixel & blue_mask;
			green = (pixel & green_mask) >> 8;
			red = (pixel & red_mask) >> 16;

			matrix_set(x, y, RGB(red, green, blue));
		}
	}
	matrix_render();
	XDestroyImage(image);

	if (frame >= FRAMES) { frame = 0; return 1;} // end drawing
	timer_add(nexttick, modno, 0, NULL);

	return 0;
}

int deinit(void) {
	return 0;
}
