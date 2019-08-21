// Simple maze animation.
// Maze algorithm by Sam "Figglewatts" Gibson: https://github.com/Figglewatts/mazegen
// Thank you kind stranger!
//
// Copyright (c) 2019, Cyrill "xermic" Leutwiler <me@09f9.org>
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
#include <stddef.h>

#include <random.h>

#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>


#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

#define INVERSE_COLORS false

#define NUM_DIRS 4
typedef struct Stack Stack; // forward declaration
static int directions[] = { 0, 1, 2, 3 };
static int* maze;
static Stack* x_stack;
static Stack* y_stack;
static int mx;
static int my;

static RGB wall = RGB(0, 0, 0);
static int modno;
static int frame;
static oscore_time nexttick;

typedef struct Stack {
	int top;
	unsigned capacity;
	int* array;
} Stack;

static inline Stack* stack_create(unsigned capacity) {
	Stack* stack = (Stack*) malloc(sizeof(Stack));
	stack->capacity = capacity;
	stack->top = -1;
	stack->array = (int*) malloc(stack->capacity * sizeof(int));
	return stack;
}

static inline bool stack_isfull(Stack* stack) {
	return stack->top == stack->capacity - 1;
}

static inline bool stack_isempty(Stack* stack) {
	return stack->top == -1;
}

static inline void stack_push(Stack* stack, int item) {
	if (stack_isfull(stack))
		return;
	stack->array[++stack->top] = item;
}

static inline int stack_pop(Stack* stack) {
	if (stack_isempty(stack))
		return INT_MIN;
	return stack->array[stack->top--];
}

static inline int* init_maze(unsigned width, unsigned height) {
	int* maze = (int*) malloc(width * height * sizeof(int));
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			maze[y * width + x] = 1;
		}
	}
	return maze;
}

static inline void shuffle(int* array, size_t n) {
	if (n > 1) {
		for (int i = 0; i < n - 1; i++) {
			int j = i + rand() / (RAND_MAX / (n - i) + 1);
			int t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

static bool carve_maze(int* maze, unsigned width, unsigned height, Stack* x_stack, Stack* y_stack) {
	shuffle(directions, NUM_DIRS);
	int x = stack_pop(x_stack);
	int y = stack_pop(y_stack);
	// hack to get a smooth animation
	bool didit = false;
	for (int i = 0; i < NUM_DIRS; i++) {
		switch (directions[i]) {
			// up
			case 0: {
				if (y - 2 <= 0) continue;
				if (maze[(y - 2) * width + x] != 0) {
					maze[(y - 2) * width + x] = 0;
					maze[(y - 1) * width + x] = 0;
					stack_push(x_stack, x);
					stack_push(y_stack, y - 2);
					didit = true;
				}
				break;
			}
			// right
			case 1: {
				if (x + 2 >= width - 1) continue;
				if (maze[y * width + x + 2] != 0) {
					maze[y * width + x + 2] = 0;
					maze[y * width + x + 1] = 0;
					stack_push(x_stack, x + 2);
					stack_push(y_stack, y);
					didit = true;
				}
				break;
			}
			// down
			case 2: {
				if (y + 2 >= height - 1) continue;
				if (maze[(y + 2) * width + x] != 0) {
					maze[(y + 2) * width + x] = 0;
					maze[(y + 1) * width + x] = 0;
					stack_push(x_stack, x);
					stack_push(y_stack, y + 2);
					didit = true;
				}
				break;
			}
			// left
			case 3: {
				if (x - 2 <= 0) continue;
				if (maze[y * width + x - 2] != 0) {
					maze[y * width + x - 2] = 0;
					maze[y * width + x - 1] = 0;
					stack_push(x_stack, x - 2);
					stack_push(y_stack, y);
					didit = true;
				}
				break;
			}
		}
	}
	return didit;
}

static inline void render_maze(int *maze, unsigned width, unsigned height) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			RGB color = HSV2RGB(HSV((y + x + frame) % 255, 255, 255));
			if (maze[y * width + x] != 0) matrix_set(x, y, INVERSE_COLORS? color: wall);
			else matrix_set(x, y, INVERSE_COLORS? wall: color);
		}
	}
	matrix_render();
}

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 8 || matrix_gety() < 8)
		return 1;
	modno = moduleno;
	frame = 0;
	return 0;
}

void reset(int _modno) {
	free(maze);
	free(x_stack);
	free(y_stack);
	// init maze
	mx = matrix_getx();
	my = matrix_gety();
	maze = init_maze(mx, my);
	x_stack = stack_create((mx * my) / 2);
	y_stack = stack_create((mx * my) / 2);
	int x = randn(mx-2)+1;
	int y = randn(my-2)+1;
	stack_push(x_stack, x);
	stack_push(y_stack, y);
	maze[y * mx + x] = 0; // start cell

	nexttick = udate();
	matrix_clear();
	frame = 0;
}


int draw(int _modno, int argc, char* argv[]) {
	while (!carve_maze(maze, mx, my, x_stack, y_stack)) {
		if (stack_isempty(x_stack) || stack_isfull(y_stack) || frame >= FRAMES) {
			frame = 0;
			return 1;
		}
	}
	render_maze(maze, mx, my);
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(maze);
	free(x_stack);
	free(y_stack);
}
