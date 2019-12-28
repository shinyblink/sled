// A digital clock.
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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "text.h"
#include "foxel35.xbm"
#include "xbm_font_loader.c"

const int font_width = 4;
const int font_height = 6;

//typedef unsigned int[5][3] char35;

// notfound = {{1,1,1},
// 			  {1,0,1},
// 			  {1,0,1},
// 			  {1,0,1},
// 			  {1,1,1}};
//int notfound = 0b111
// unsigned int notfound[5][3]= {{1,1,1},
//  			  {1,0,1},
//  			  {1,0,1},
//  			  {1,0,1},
//  			  {1,1,1}};
//#define unknownCharacter {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}



#define FRAMETIME (T_SECOND)
#define FRAMES (TIME_SHORT)
#define CHARS_FULL 8 // 20:15:09, must also hold 20:15 (small)

struct font_char{
	char c;
	RGB fg;
	RGB bg;
};

static oscore_time nexttick;
static int frame = 0;
static int moduleno;
static int usesmall;
static char clockstr[CHARS_FULL + 1];
static text* rendered = NULL;
static int max_row;
static int max_column;
struct font_char* buffer;

int init (int modno, char* argstr) {
	moduleno = modno;
	int i;
	//for(i = 0; i < 32; ++i)
	//	foxel35[i] = notfound;

	max_row = matrix_gety() / 6;
	max_column = matrix_getx() / 4;

	buffer = malloc(max_row * max_column * sizeof(struct font_char));
	for(i=0; i< max_row * max_column; ++i){
		buffer[i].c='A';
		buffer[i].fg=RGB(255,255,255);
		buffer[i].bg=RGB(0,0,0);
	}

	//2 rows
	if (matrix_getx() < 11)
		return 1; // not enough X to be usable
	// 8 characters
	if (matrix_gety() < 16)
		return 1; // not enough Y to be usable
	// max digit size is 4, plus 3 for :, so 4 + 4 + 3 + 4 + 4 + 3 + 4 + 4 = 24 + 6 = 30
	usesmall = matrix_getx() < 30;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	time_t rawtime;
	struct tm * timeinfo;
	const char * format = "%T";
	unsigned char ch = 0;
	int x = 0;
	int y = 0;
	int row = 0;
	int column = 0;
	RGB fg = RGB(247,127,190);
	RGB bg = RGB(50,50,50);
	for(row = 0; row < max_row; ++row)
		for(column = 0; column < max_column; ++column){
			for (y = 0; y < 6; ++y){
				for (x = 0; x < 4; ++x) {
					matrix_set((column*4) + x,(row*6) + y, (load_char(foxel35_bits, ch,x,y,font_width,font_height) == 1?fg:bg));
					
				}
			}
			ch++;
			if(ch >= 128){
				ch = 0;
			}
		}

	matrix_render();
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	// This acts conditionally on rendered being non-NULL.
	text_free(rendered);
}



