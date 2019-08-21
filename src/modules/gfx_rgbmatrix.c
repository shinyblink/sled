/*
 * rgbmatrix port, much stolen from sinematrix_rgb() at https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/MatrixDemo.ino
 *
 * This is an effect that basically paints a "base canvas" via a function and then defines a "camera" that
 * moves, rotates and zooms seemingly randomly over that canvas, showing what that "camera" sees.
 * This effect uses three such cameras, one for each RGB color, blending the result together.
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

// basic scale factor, determining how much of the "base image" is seen on average.
// Higher numbers mean that the camera is farther away from the canvas.
static const float scale_factor = 4*M_PI;

/*** module run variables (internal state) ***/

// primary matrix coefficient run variables
static float pangle = 0;      // "proto-angle"
static float angle = 0;       // rotation angle
static float sx = 0;          // scale factor x
static float sy = 0;          // scale factor y
static float tx = 0;          // translation x
static float ty = 0;          // translation y
static float cx = 0;          // center offset x (used for rcx)
static float cy = 0;          // center offset y (used for rcy)
static float rcx = 0;         // rotation center offset x
static float rcy = 0;         // rotation center offset y

// secondary set of matrix coefficient run variables (used for "shadow overlay" in sinematrix2)
static float pangle2 = 0;      // "proto-angle"
static float angle2 = 0;      // angle 
static float sx2 = 0;         // scale factor x
static float sy2 = 0;         // scale factor y
static float tx2 = 0;         // translation x
static float ty2 = 0;         // translation y

static float cx2 = 0;          // center offset x (used for rcx)
static float cy2 = 0;          // center offset y (used for rcy)
static float rcx2 = 0;         // rotation center offset x
static float rcy2 = 0;         // rotation center offset y

static float pangle3 = 0;      // "proto-angle"
static float angle3 = 0;       // rotation angle
static float sx3 = 0;          // scale factor x
static float sy3 = 0;          // scale factor y
static float tx3 = 0;         // translation x
static float ty3 = 0;         // translation y
static float cx3 = 0;          // center offset x (used for rcx)
static float cy3 = 0;          // center offset y (used for rcy)
static float rcx3 = 0;         // rotation center offset x
static float rcy3 = 0;         // rotation center offset y

static float basecol = 0;     // base color offset to start from for each frame


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
	modno = moduleno;
	return 0;
}

/*** base "image" function ***/

static inline float halfsines3D(float x, float y) {
  float r = (cos(x) * sin(y));
  return (r > 0.0 ? r : 0.0);
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

	pangle = addmodpi( pangle, 0.00133 + (angle/256) );
	angle = cos(pangle) * M_PI;
	pangle2 = addmodpi( pangle2, 0.00323 + (angle2/256) );
	angle2 = cos(pangle2) * M_PI;
	pangle3 = addmodpi( pangle3, 0.00613 + (angle3/256) );
	angle3 = cos(pangle3) * M_PI;
	sx = addmodpi( sx, 0.000673 );
	sy = addmodpi( sy, 0.000437 );
	sx2 = addmodpi( sx2, 0.000973 );
	sy2 = addmodpi( sy2, 0.001037 );
	sx3 = addmodpi( sx3, 0.00273 );
	sy3 = addmodpi( sy3, 0.00327 );
	tx = addmodpi( tx, 0.000239 );
	ty = addmodpi( ty, 0.000293 );
	tx2 = addmodpi( tx2, 0.000439 );
	ty2 = addmodpi( ty2, 0.000193 );
	tx3 = addmodpi( tx3, 0.000639 );
	ty3 = addmodpi( ty3, 0.000793 );
	cx = addmodpi( cx, 0.000347 );
	cy = addmodpi( cy, 0.000437 );
	rcx = (mx/2) + (sin(cx2) * mx);
	rcy = (my/2) + (sin(cy2) * my);
	cx2 = addmodpi( cx2, 0.000697 );
	cy2 = addmodpi( cy2, 0.000727 );
	rcx2 = (mx/2) + (sin(cx2) * mx);
	rcy2 = (my/2) + (sin(cy2) * my);
	cx3 = addmodpi( cx3, 0.000197 );
	cy3 = addmodpi( cy3, 0.000227 );
	rcx3 = (mx/2) + (sin(cx3) * mx);
	rcy3 = (my/2) + (sin(cy3) * my);
	basecol = addmod( basecol, 1.0, 0.007 );
	
	matrix2_2 rotater = {
		.v1_1 = cos(angle),
		.v1_2 = -sin(angle),
		.v2_1 = sin(angle),
		.v2_2 = cos(angle)
	};
	matrix2_2 rotateg = {
		.v1_1 = cos(angle2),
		.v1_2 = -sin(angle2),
		.v2_1 = sin(angle2),
		.v2_2 = cos(angle2)
	};
	matrix2_2 rotateb = {
		.v1_1 = cos(angle3),
		.v1_2 = -sin(angle3),
		.v2_1 = sin(angle3),
		.v2_2 = cos(angle3)
	};
	matrix2_2 scaler = {
		.v1_1 = (sin(sx) + 1.0)/fx,
		.v1_2 = 0,
		.v2_1 = 0,
		.v2_2 = (cos(sy) + 1.0)/fy
	};
	matrix2_2 scaleg = {
		.v1_1 = (sin(sx2) + 1.0)/fx,
		.v1_2 = 0,
		.v2_1 = 0,
		.v2_2 = (cos(sy2) + 1.0)/fy
	};
	matrix2_2 scaleb = {
		.v1_1 = (sin(sx3) + 1.0)/fx,
		.v1_2 = 0,
		.v2_1 = 0,
		.v2_2 = (cos(sy3) + 1.0)/fy
	};
	vec2 translater = {
		.x = sin(tx) * mx,
		.y = sin(ty) * my
	};
	vec2 translateg = {
		.x = sin(tx2) * mx,
		.y = sin(ty2) * my
	};
	vec2 translateb = {
		.x = sin(tx3) * mx,
		.y = sin(ty3) * my
	};

	for( int x = 0; x < mx; x++ ) {
		for( int y = 0; y < my; y++ ) {
			vec2 rc =  { .x = x-rcx,  .y = y-rcy  };
			vec2 rc2 = { .x = x-rcx2, .y = y-rcy2 };
			vec2 rc3 = { .x = x-rcx3, .y = y-rcy3 };
			vec2 cr = vadd(multm2v2( multm2(rotater, scaler), rc ), translater);
			vec2 cg = vadd(multm2v2( multm2(rotateg, scaleg), rc2 ), translateg);
			vec2 cb = vadd(multm2v2( multm2(rotateb, scaleb), rc3 ), translateb);
			RGB col = RGB(
				(halfsines3D(cr.x, cr.y))*255, 
				(halfsines3D(cg.x, cg.y))*255, 
				(halfsines3D(cb.x, cb.y))*255 
			);
			matrix_set(x, y, col);
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
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

/*** module deconstructor ***/

void deinit(int _modno) {}
