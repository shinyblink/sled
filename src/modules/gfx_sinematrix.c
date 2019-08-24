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
#define FRAMES (TIME_LONG * FPS)

/*** management variables ***/

static int modno;
static int frame = 0;
static oscore_time nexttick;

/*** matrix info (initialized in init()) ***/

static int mx, my;		// matrix size
static float fx, fy;		// size-dependent factors

/*** base effect coefficients. This is where you want to play around. ***/

// basic scale factor, determining how much of the "base image" is seen on average.
// Higher numbers mean that the camera is farther away from the canvas.
static const float scale_factor = M_PI;

// Increments per frame. Higher numbers mean higher speed, but keep these values well below 0.1.
// Anything higher than 0.1 definitely produces visual stuttering and blinking.
// Using different numbers for each value (e.g. fractions of primes) avoids pattern repetition.
static const float inc_basecolor =	0.007f;		// basecolor changing speed. Increase this for more trippyness.
static const float inc_angle = 	0.00937f;	// camera rotation speed
static const float var_angle = 	0.0039f;	// add a bit of variance to the above speed, i.e. plusminus up to this amount
static const float inc_scale_x =	0.00673f;	// camera scale factor changing speed in X direction
static const float inc_scale_y =	0.00437f;	// camera scale factor changing speed in Y direction
static const float inc_translate_x =	0.0239f;	// camera movement speed in X direction
static const float inc_translate_y =	0.0293f;	// camera movement speed in Y direction
static const float inc_rotcenter_x =	0.00197f;	// camera rotation center changing speed in X direction
static const float inc_rotcenter_y =	0.00227f;	// camera rotation center changing speed in Y direction

// maximum effect color range per frame. Values between 32 and 1024 are interesting here.
// Values above 255 let the rainbow overflow inside the effect range... well, it's hard to describe ^^
static const int effect_color_range = 	255;

// middle of the scale factor range (the scale factors will revolve around these values). Keep them between 0.0 and 1.0.
static const float offset_scale_x =	0.35f;
static const float offset_scale_y =	0.35f;

/*** module run variables (internal state) ***/

// primary matrix coefficient run variables
static float pangle = 0;	// "proto-angle" (used for rotation angle)
static float angle = 0;	// rotation angle
static float sx = 0;		// scale factor x
static float sy = 0;		// scale factor y
static float tx = 0;		// translation x
static float ty = 0;		// translation y
static float cx = 0;		// center offset x (used for rcx)
static float cy = 0;		// center offset y (used for rcy)
static float rcx = 0;		// rotation center offset x
static float rcy = 0;		// rotation center offset y
static float basecol = 0; 	// base color offset to start from for each frame

/*** optimization preinit ***/

static RGB precalc_hsv[256]; // precalculated X, 255, 255

/*** module init ***/

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();
	fx = mx / scale_factor;
	fy = my / scale_factor;
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;
	for (int i = 0; i < 256; i++)
		precalc_hsv[i] = HSV2RGB(HSV(i, 255, 255));
	modno = moduleno;
	return 0;
}

/*** base "image" function ***/

static inline float sinecircle3D(float x, float y) {
	return (cosf(x) * sinf(y) * cosf(sqrtf((x*x) + (y*y))));
}

/*** math helper functions ***/

static inline float addmod(float x, float mod, float delta) {
	x = fmodf(x + delta, mod);
	x += x<0 ? mod : 0;
	return x;
}

static inline float addmodpi(float x, float delta) {
	return addmod(x, 2 * M_PI, delta);
}

/*** main drawing loop ***/

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	nexttick = udate() + FRAMETIME;
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
	matrix2_2 rotate = {
	  .v1_1 = cosf(angle),
	  .v1_2 = -sinf(angle),
	  .v2_1 = sinf(angle),
	  .v2_2 = cosf(angle),
	};
	matrix2_2 scale = {
	  .v1_1 = (sinf(sx) + offset_scale_x)/fx,
	  .v1_2 = 0,
	  .v2_1 = 0,
	  .v2_2 = (cosf(sy) + offset_scale_y)/fy,
	};
	vec2 translate = {
	  .x = sinf(tx) * mx,
	  .y = sinf(ty) * my,
	};

	matrix2_2 rotscale = multm2(rotate, scale);
	vec2 rotscale_xbasis = {
	  .x = rotscale.v1_1,
	  .y = rotscale.v2_1,
	};
	vec2 rotscale_ybasis = {
	  .x = rotscale.v1_2,
	  .y = rotscale.v2_2,
	};

	// put it all together
	int x;
	int y;
	vec2 outerbasis = {
	  .x = (rotscale_xbasis.x * -rcx) + (rotscale_ybasis.x * -rcy),
	  .y = (rotscale_xbasis.y * -rcx) + (rotscale_ybasis.y * -rcy)
	};
	outerbasis = vadd(outerbasis, translate);
	for (y = 0; y < my; ++y) {
		vec2 c = outerbasis;
		for (x = 0; x < mx; ++x) {
			// vec2 c = vadd(vmmult(rotscale, (vector) { .x = x-rcx, .y = y-rcy }), translate);
			float hue = (basecol * 255) + (sinecircle3D(c.x, c.y) * effect_color_range);
			matrix_set(x, y, precalc_hsv[(((int) hue) & 0xFF)]);
			c = vadd(c, rotscale_xbasis);
		}
		outerbasis = vadd(outerbasis, rotscale_ybasis);
	}

	// render it out
	matrix_render();

	// manage framework variables
	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

/*** module deconstructor ***/

void deinit(int _modno) {}
