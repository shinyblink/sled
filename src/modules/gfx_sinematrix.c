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
static int frame;
static ulong nexttick;

/*** matrix info (initialized in init()) ***/

int mx, my;		// matrix size
float fx, fy;		// size-dependent factors

/*** base effect coefficients. This is where you want to play around. ***/

const float scale_factor = M_PI;		// basic scale factor, determining how much of the "base image" is seen on average.

// Increments per frame. Higher numbers mean higher speed, but keep these values well below 0.1.
// Anything higher than 0.1 definitely produces visual stuttering and blinking.
// Using different numbers for each value (e.g. fractions of primes) avoids pattern repetition.
const float inc_basecolor =	0.007f;		// basecolor changing speed. Increase this for more trippyness.
const float inc_angle = 	0.00937f;	// camera rotation speed
const float var_angle = 	0.0039f;	// add a bit of variance to the above speed, i.e. plusminus up to this amount
const float inc_scale_x =	0.00673f;	// camera scale factor changing speed in X direction
const float inc_scale_y =	0.00437f;	// camera scale factor changing speed in Y direction
const float inc_translate_x =	0.0239f;	// camera movement speed in X direction
const float inc_translate_y =	0.0293f;	// camera movement speed in Y direction 
const float inc_rotcenter_x =	0.00197f;	// camera rotation center changing speed in X direction
const float inc_rotcenter_y =	0.00227f;	// camera rotation center changing speed in Y direction


// middle of the scale factor range (the scale factors will revolve around these values). Keep them between 0.0 and 1.0.
const float offset_scale_x =	0.35f;		
const float offset_scale_y =	0.35f;

/*** module run variables (internal state) ***/

// primary matrix coefficient run variables
float pangle = 0;	// "proto-angle" (used for rotation angle)
float angle = 0;	// rotation angle
float sx = 0;		// scale factor x
float sy = 0;		// scale factor y
float tx = 0;		// translation x
float ty = 0;		// translation y
float cx = 0;		// center offset x (used for rcx)
float cy = 0;		// center offset y (used for rcy)
float rcx = 0;		// rotation center offset x
float rcy = 0;		// rotation center offset y
float basecol = 0; 	// base color offset to start from for each frame


/*** module init ***/

int init(int moduleno) {
	mx = matrix_getx();
	my = matrix_gety();
	fx = mx / scale_factor;
	fy = my / scale_factor;
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;

	modno = moduleno;
	return 0;
}

/*** base "image" function ***/

inline float sinecircle3D(float x, float y) {
	return (cosf(x) * sinf(y) * cosf(sqrtf((x*x) + (y*y))));
}

/*** math helper functions ***/

inline float addmod(float x, float mod, float delta) {
	x = x + delta;
	while( x >= mod ) x -= mod;
	while( x <  0.0 ) x += mod;
	return x;
}

inline float addmodpi(float x, float delta) {
	return addmod(x, 2 * M_PI, delta);
}

/*** main drawing loop ***/

int draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = udate();
	}

	matrix_clear();

	// increase the run variables by the const amounts set above
	pangle = addmodpi( pangle, inc_angle + (angle*var_angle) );
	angle = cosf(pangle) * M_PI;
	sx = addmodpi( sx, inc_scale_x );
	sy = addmodpi( sy, inc_scale_y );
	tx = addmodpi( tx, inc_translate_x/mx );
	ty = addmodpi( ty, inc_translate_y/my );
	cx = addmodpi( cx, inc_rotcenter_x );
	cy = addmodpi( cy, inc_rotcenter_y );
	rcx = (mx/2) + (sinf(cx) * mx);
	rcy = (my/2) + (sinf(cy) * my);
	basecol = addmod( basecol, 1.0, inc_basecolor );

	// construct matrices, using run variables as coefficients
	// (sometimes normalized and made more interesting by sine curves)
	matrix rotate = {
	  .v1_1 = cosf(angle),
	  .v1_2 = -sinf(angle),
	  .v2_1 = sinf(angle),
	  .v2_2 = cosf(angle),
	};
	matrix scale = {
	  .v1_1 = (sinf(sx) + offset_scale_x)/fx,
	  .v1_2 = 0,
	  .v2_1 = 0,
	  .v2_2 = (cosf(sy) + offset_scale_y)/fy,
	};
	vector translate = {
	  .x = sinf(tx) * mx,
	  .y = sinf(ty) * my,
	};

	// put it all together
	int x;
	int y;
	for (y = 0; y < my; ++y)
		for (x = 0; x < mx; ++x) {
			vector c = vadd(vmmult(mmult(rotate, scale), (vector) { .x = x-rcx, .y = y-rcy }), translate);
			RGB col = HSV2RGB(HSV((basecol+sinecircle3D(c.x, c.y))*255, 255, 255));
			matrix_set(x, y, &col);
		};

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
