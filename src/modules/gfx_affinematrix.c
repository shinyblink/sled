/*
 * sinematrix port, much stolen from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/MatrixDemo.ino
 *
 * This is an effect that basically paints a "base canvas" via a function and then defines a "camera" that
 * moves, rotates and zooms seemingly randomly over that canvas, showing what that "camera" sees.
 */

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <mathey.h>
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS)

/*** management variables ***/

static int modno;
static int frame = 0;
static ulong nexttick;

/*** matrix info (initialized in init()) ***/

int mx, my;		// matrix size

/*** base effect coefficients. This is where you want to play around. ***/

#define runvar_count 16
static float runinc[runvar_count] = { 
  0.00123,   0.00651,  0.000471,  0.000973,   
  0.000223,  0.000751,  0.000879,  0.000443,   
  0.000373,  0.000459,  0.000321,  0.000247,   
  0.000923,  0.00253,  0.00173,  0.000613   
};
float runvar[runvar_count] = { 
  0,        0,        0,        0, 
  0,        0,        0,        0, 
  0,        0,        0,        0, 
  0,        0,        0,        0 
};
static float runmod[runvar_count] = { 
  1.0,      2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI
};


/*** module init ***/

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;
	modno = moduleno;
	printf("affinematrix init\n");
	return 0;
}

void reset() {
	nexttick = udate();
	frame = 0;
}

inline float addmod(float x, float mod, float delta) {
  x = x + delta;
  while( x >= mod ) {
    x -= mod;
  }
  while( x <  0.0 ) {
    x += mod;
  }
  return x;
}

void increment_runvars() {
  for( int i = 0; i < runvar_count; i++ ) {
    runvar[i] = addmod(runvar[i], runmod[i], runinc[i]);
  }
}

inline float _abs(float x) {
  if( x < 0 ) {
    return -x;
  } else {
    return x;
  }
}

float sinestuff(float x, float y, float v0, float v1) {
  return ( cosf(v1+x) * sinf(v1+y) * cosf((mx*0.5*sinf(v0)) + sqrtf(x*x + y*y)) );
}

inline float sinecircle3D(float x, float y) {
        return (cosf(x) * sinf(y) * cosf(sqrtf((x*x) + (y*y))));
}

inline int _min(int x, int y) {
	return x>y ? y : x;
}


int draw(int argc, char* argv[]) {
	increment_runvars();
	matrix3_3 m = composem3( 9,
		rotation3(cos(runvar[12]) * M_PI),
		translation3(cos(runvar[2])*mx*0.125, sin(runvar[3])*my*0.125),
		scale3(16.0/mx, 16.0/my),
		rotation3(runvar[13]),
		translation3(sin(runvar[4])*mx*0.25, cos(runvar[5])*my*0.25),
		rotation3(sin(runvar[14]) * M_PI),
		translation3(sin(runvar[6])*mx*0.125, cos(runvar[7])*my*0.125),
		rotation3(runvar[15]),
		scale3(0.25+sin(runvar[8])/6.0, 0.25+cos(runvar[9])/6.0) 
	); 
	for( int x = 0; x < mx; x++ ) {
		for( int y = 0; y < my; y++ ) {
			vec2 v = multm3v2(m, vec2(x-(mx/2), y-(mx/2)));
			float sc = sinestuff(v.x, v.y, runvar[10], runvar[11]);
			float hue = runvar[0] + cosf(runvar[1]) + (sc * 0.5);
			RGB color = HSV2RGB(HSV( ((int)(hue*255) & 0xFF), 255, _min(255,(int)(_abs(sc)*512)) ));
			matrix_set(x,y, &color);
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

int deinit() {
	return 0;
}
