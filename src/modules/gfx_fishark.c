// Port of https://github.com/aperifons/wa-tor
//
// Copyright (c) 2020, Sebastian "basxto" Riedel <git@basxto.de>
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
#include <stdio.h>


#define STEPS 4 // fair to assume most matrices can be divided by 4.
#define FRAMETIME (T_SECOND / STEPS)
#define FRAMES (TIME_SHORT * STEPS)
#define STEP_X (matrix_getx() / STEPS / 2)
#define STEP_Y (matrix_gety() / STEPS / 2)

static int modno;
static int step = 0;
static int dir = 1;
static int frame = 0;
static oscore_time nexttick;

static int* table;
static int* table_copy;
static int width;
static int height;

int init(int moduleno, char* argstr) {
	width = matrix_getx();
	height = matrix_gety();
	modno = moduleno;
	table = malloc(width * height * sizeof(int));
	table_copy = malloc(width * height * sizeof(int));
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	int rand;
	for(int i = 0; i < width*height; ++i){
		rand = randn(10);
		if(rand < 7)
			table[i] = 0;
		else if(rand >= 9)
			table[i] = 2; //shark
		else
			table[i] = 1; // fish
		table_copy[i] = 0;	
	}
}

// 0 both free
// 1 one has a fish
int point_free(int x,int y){
	int index = (y*width) + x;
	return table[index] | table_copy[index];
}

void move_point(int oldx, int oldy, int x, int y){
	table[(oldy*width) + oldx] = 0;
	table[(y*width) + x] = table_copy[(oldy*width) + oldx];
	table_copy[(oldy*width) + oldx] = 0;
	table_copy[(y*width) + x] = 0;
}

void move_fishark(){
	int* tmp = table_copy;
	table_copy = table;
	table = tmp;
// 1 0 2
// 3 X 4
// 5 7 6
	for(int y = 0; y < height; ++y){
		for(int x = 0; x < width; ++x){
			if(table_copy[(y * width) + x] > 0){
				int rand = randn(8);
				int x_new = x;
				int y_new = y;
				if(table_copy[(y * width) + x] > 1){
					if(point_free(x - 1 ,y - 1) == 1){
						x_new = x - 1;
						y_new = y - 1;
					}
					if(point_free(x ,y - 1) == 1){
						x_new = x;
						y_new = y - 1;
					}
					if(point_free(x + 1 ,y - 1) == 1){
						x_new = x + 1;
						y_new = y - 1;
					}
					if(point_free(x - 1 ,y) == 1){
						x_new = x - 1;
						y_new = y;
					}
					if(point_free(x ,y) == 1){
						x_new = x - 1;
						y_new = y;
					}
					if(point_free(x + 1,y) == 1){
						x_new = x + 1;
						y_new = y;
					}
					if(point_free(x - 1,y + 1) == 1){
						x_new = x - 1;
						y_new = y + 1;
					}
					if(point_free(x ,y + 1) == 1){
						x_new = x - 1;
						y_new = y + 1;
					}
					if(point_free(x + 1,y + 1) == 1){
						x_new = x + 1;
						y_new = y + 1;
					}
				}
				if(x_new == x && y_new == y){
					if( rand <= 2){
						--y_new;
					}
					if(rand >= 5){
						++y_new;
					}
					if(rand > 0 && rand < 7){
						if(rand%2){
							++x_new;
						} else {
							--x_new;
						}
					}
					if(y_new < 0)
						y_new = height - y_new;
					if(x_new < 0)
						x_new = width - x_new;
					y_new = y_new % height;
					x_new = x_new % width;

					if(point_free(x_new, y_new) > 0){
						x_new = x;
						y_new = y;
					}
				}
				move_point(x, y, x_new, y_new);
			}
		}
	}
}

int draw(int _modno, int argc, char* argv[]) {
	move_fishark();
	matrix_clear();
	for(int y = 0; y < height; ++y){
		for(int x = 0; x < width; ++x){
			RGB col = RGB(255,255,255);
			int index = (width*y) + x;
			if(table[index] == 1){
				col = RGB(0,255,0);
			} else if(table[index] > 1) {
				col = RGB(255,0,0);
			}
			matrix_set(x,y,col);
		}
	}
	matrix_render();
	frame++;
	step += dir;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
