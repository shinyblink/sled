// Matrix-style lines moving from top to bottom.
// However, in random colors!
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
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

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx;
static int my;

typedef struct line {
	RGB color;
	int pos_x;
	int pos_y;
	int speed;
	int length;
} line;

static int numlines;
static line* lines;

static RGB black = RGB(0, 0, 0);

static void randomize_line(int line) {
	lines[line].color = HSV2RGB(HSV(randn(255), 255, 255)); // HSV instead of RGB to get sameish brightness but differing color.

	lines[line].pos_y = -randn(my/4);
	lines[line].pos_x = randn(mx - 1);

	int speed = 0;
	while (speed == 0)
		speed = randn(2);
	lines[line].speed = speed;

	int length = 0;
	while (length < (my/4))
		length = randn(my/2);
	lines[line].length = length;
}

static void randomize_lines() {
	int line;
	for (line = 0; line < numlines; ++line) {
		randomize_line(line);
	}
}

static void update_lines() {
	int line;
	int y;
	for (line = 0; line < numlines; ++line) {
		y = lines[line].pos_y + lines[line].speed;

		if ((y - lines[line].length) > my) {
			// clear old line
			/*int py;
			for (py = y; py >= (y - lines[line].length); py--)
				if (py < my)
				matrix_set(lines[line].pos_x, py, black);*/
			randomize_line(line);
		} else {
			lines[line].pos_y = y;
		}
	}
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

	numlines = mx / 4; // not sure if this is the best thing to do, but meh.
	lines = malloc(numlines * sizeof(line));

	randomize_lines();

	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();

	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	int line;
	// clear out old points of lines
	for (line = 0; line < numlines; ++line) {
		int y = lines[line].pos_y - lines[line].length;
		int speed;
		for (speed = 0; speed <= lines[line].speed; speed++)
			if ((y - speed) >= 0 && (y - speed) < my)
				matrix_set(lines[line].pos_x, y - speed, black);
	}

	// update the lines and draw them
	update_lines(); // todo, move back below matrix_render, to get a more consistant framerate
	for (line = 0; line < numlines; ++line) {
		int y;
		for (y = lines[line].pos_y - lines[line].length; y <= lines[line].pos_y; y++)
			if (y >= 0 && y < my)
				matrix_set(lines[line].pos_x, y, lines[line].color);
	}

	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(lines);
}
