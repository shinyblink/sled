// "The movement of the string, always holding to form yet advancing, like a river ahead of a newly broken dam, was fascinating."
// another module by 20kdc - this one's specifically for the microbit!

// uses a very dumb player which is guaranteed to eventually lose
// it's rules are:
// 1. if we are left/right of the target, make a turn
// 2. otherwise, continue advancing in whatever the current direction is
// 2. we grow the snake by SNEK_GROWTH per target reached ; it's calibrated to make the limit 5 rounds
// 3. if the snake is going to outgrow the position buffer, we kill it instead

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
	int x;
	int y;
} snek_position_t;

#define SNEK_BUFFERMUL 3
#define SNEK_GROWTH ((snek_max_length + 19) / 20)
#define SNEK_FRAMETIME (((50 * T_MILLISECOND) / SNEK_GROWTH) + 1)

#define SNEK_MOVE_FRAME_MASK 3
#define SNEK_TARGET_FRAME_MASK 1

#define SNEK_TARGET RGB(0, 255, 0)
#define SNEK_BODY RGB(255, 255, 255)

#define SNEK_ACCUMULATOR_TO_GROW 2

static int snek_moduleno;
static oscore_time snek_nexttick;

// game state

// snake's body; [0] is head of snake (the part that is moving)
static snek_position_t * snek_points;
// this is carried over from last game
static snek_position_t snek_movement = {
	.x = 1,
	.y = 0,
};
static int snek_length, snek_max_length;
static int snek_grow_frames;
static int snek_grow_accumulator;
static int snek_blink_frame;
static snek_position_t snek_target;

#define SNEK_GS_ALIVE 0
#define SNEK_GS_DEAD 1
static int snek_game_state;

int init(int moduleno, char* argstr) {
	snek_max_length = matrix_getx();
	int my = matrix_gety();
	if (my < snek_max_length)
		snek_max_length = my;
	if (snek_max_length == 0)
		return 1;
	// stop the snek from "mysteriously dying" too often by the safety-net
	snek_max_length *= SNEK_BUFFERMUL;
	snek_points = malloc(snek_max_length * sizeof(snek_position_t));
	assert(snek_points);
	snek_moduleno = moduleno;
	// set working but not-good values (reset SHOULD be called)
	snek_nexttick = udate();
	snek_grow_frames = 0;
	snek_grow_accumulator = 0;
	snek_blink_frame = 0;
	snek_game_state = SNEK_GS_ALIVE;
	snek_length = 1;
	snek_points[0].x = 0;
	snek_points[0].y = 0;
	snek_target.x = 0;
	snek_target.y = 0;
	return 0;
}

void reset(int _modno) {
	snek_nexttick = udate();
	snek_grow_frames = 0;
	snek_grow_accumulator = 0;
	snek_game_state = SNEK_GS_ALIVE;
	snek_length = 1;
	snek_points[0].x = randn(matrix_getx() - 1);
	snek_points[0].y = randn(matrix_gety() - 1);
	snek_target.x = randn(matrix_getx() - 1);
	snek_target.y = randn(matrix_gety() - 1);
	// draw initial board state
	matrix_clear();
	matrix_set(snek_points[0].x, snek_points[0].y, SNEK_BODY);
	matrix_set(snek_target.x, snek_target.y, SNEK_TARGET);
}

static void snek_wrap_position(snek_position_t * pos) {
	int mx = matrix_getx();
	int my = matrix_gety();
	if (pos->x < 0)
		pos->x = mx - 1;
	else if (pos->x >= mx)
		pos->x = 0;

	if (pos->y < 0)
		pos->y = my - 1;
	else if (pos->y >= my)
		pos->y = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	int mx = matrix_getx();
	int my = matrix_gety();
	if (snek_blink_frame & SNEK_MOVE_FRAME_MASK) {
		// we don't do anything this frame
	} else if (snek_game_state == SNEK_GS_ALIVE) {
		// steering
		if ((snek_movement.x != 0) && (snek_points[0].x == snek_target.x)) {
			snek_movement.x = 0;
			snek_movement.y = randn(1) ? -1 : 1;
		} else if ((snek_movement.y != 0) && (snek_points[0].y == snek_target.y)) {
			snek_movement.x = randn(1) ? -1 : 1;
			snek_movement.y = 0;
		}
		// position advance, part 1
		snek_position_t next = {
			.x = snek_points[0].x + snek_movement.x,
			.y = snek_points[0].y + snek_movement.y,
		};

		snek_wrap_position(&next);

		// target collision check
		if ((next.x == snek_target.x) && (next.y == snek_target.y)) {
			// the old target will be overwritten anyway
			snek_grow_accumulator++;
			if (snek_grow_accumulator >= SNEK_ACCUMULATOR_TO_GROW) {
				snek_grow_accumulator = 0;
				snek_grow_frames += SNEK_GROWTH;
			}
			if (randn(1) == 0) {
				// nefarious target selection
				snek_target.x = next.x + (snek_movement.y * 2);
				snek_target.y = next.y + (snek_movement.x * 2);
				snek_wrap_position(&snek_target);
			} else {
				snek_target.x = randn(mx - 1);
				snek_target.y = randn(my - 1);
			}
		}
		// snake collision check
		for (int i = 0; i < snek_length; i++) {
			if ((snek_points[i].x == next.x) && (snek_points[i].y == next.y)) {
				snek_game_state = SNEK_GS_DEAD;
				break;
			}
		}
		// if the snake survived...
		if (snek_game_state == SNEK_GS_ALIVE) {
			if (snek_grow_frames) {
				snek_grow_frames--;
				// advance behavior: growing forward
				if (snek_length == snek_max_length) {
					// ran out of room; die instantly
					snek_game_state = SNEK_GS_DEAD;
				} else {
					// advance!
					memmove(snek_points + 1, snek_points, snek_length * sizeof(snek_position_t));
					snek_points[0] = next;
					snek_length++;
					matrix_set(snek_points[0].x, snek_points[0].y, SNEK_BODY);
				}
			} else {
				// advance behavior: non-growing forward
				matrix_set(snek_points[snek_length - 1].x, snek_points[snek_length - 1].y, RGB(0, 0, 0));
				matrix_set(next.x, next.y, SNEK_BODY);
				memmove(snek_points + 1, snek_points, (snek_length - 1) * sizeof(snek_position_t));
				snek_points[0] = next;
			}
		}
	} else if (snek_game_state == SNEK_GS_DEAD) {
		// remove snake components from the end forwards
		if (snek_length == 0)
			return 1;
		snek_length--;
		matrix_set(snek_points[snek_length].x, snek_points[snek_length].y, RGB(0, 0, 0));
	}
	snek_blink_frame++;
	if ((snek_blink_frame & SNEK_TARGET_FRAME_MASK) == 0) {
		matrix_set(snek_target.x, snek_target.y, SNEK_TARGET);
	} else {
		matrix_set(snek_target.x, snek_target.y, RGB(0, 0, 0));
	}
	matrix_render();
	snek_nexttick += SNEK_FRAMETIME;
	timer_add(snek_nexttick, snek_moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(snek_points);
}
