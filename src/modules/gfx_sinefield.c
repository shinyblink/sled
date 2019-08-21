/*
 * sinefield port, much stolen from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/MatrixDemo.ino
 *
 * Even the original author, @orithena, has no clue how this effect works. He just experimented and this came out.
 *
 */

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <mathey.h>
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

/*** management variables ***/

static int modno;
static int frame = 0;
static oscore_time nexttick;

/*** matrix info (initialized in init()) ***/

static int mx, my;		// matrix size

/*** module init ***/

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;
	modno = moduleno;
	return 0;
}


void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

/*** main drawing loop ***/

int draw(int _modno, int argc, char* argv[]) {
	float step = (float)(((udate()) >> 16) & 0x00007FFF);
	//printf("[%8.1f]", step);
	byte hue = 0;
	for(int x = 0; x < mx; x++ ) {
		hue = step + (37 * sinf( ((x*step)/(11*M_PI)) * 0.04 ) );
		for(int y = 0; y < my; y++ ) {
			hue += 17 * sinf(y/(5*M_PI));
			RGB color = HSV2RGB(HSV(
				(byte)(hue + (byte)step),
				255, //(byte)(192 - (63*cosf((hue+step)*M_PI*0.004145))),
				(byte)(255*sinf((hue+step)*M_PI*0.003891))
			));
			matrix_set(x,y,color);
		}
	}

	// render it out
	matrix_render();

	// manage framework variables
	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

/*** module deconstructor ***/

void deinit(int _modno) {}
