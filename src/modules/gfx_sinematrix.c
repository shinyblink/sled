// sinematrix port, much stolen from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/MatrixDemo.ino

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <stddef.h>
#include <mathey.h>
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS)

static int modno;
static int frame;
static ulong nexttick;

// primary matrix coefficient run variables
float pangle = 0;      // "proto-angle"
float angle = 0;       // rotation angle
float sx = 0;          // scale factor x
float sy = 0;          // scale factor y
float tx = 0;          // translation x
float ty = 0;          // translation y
float cx = 0;          // center offset x (used for rcx)
float cy = 0;          // center offset y (used for rcy)
float rcx = 0;         // rotation center offset x
float rcy = 0;         // rotation center offset y

// secondary set of matrix coefficient run variables (used for "shadow overlay" in sinematrix2)
float angle2 = 0;      // angle
float sx2 = 0;         // scale factor x
float sy2 = 0;         // scale factor y
float tx2 = 0;         // translation x
float ty2 = 0;         // translation y

float basecol = 0; // base color offset to start from for each frame

int init(int moduleno) {
	int mx = matrix_getx();
	int my = matrix_gety();
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;

	modno = moduleno;
	return 0;
}

inline float sines3D(float x, float y) {
	return ((cosf(x) * sinf(y)) * 0.5) + 0.5;
}

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

int draw(int argc, char* argv[]) {
	if (frame == 0) {
		nexttick = udate();
	}

	matrix_clear();

	const int mx = matrix_getx();
	const int my = matrix_gety();
	const float size_factor = M_PI;
	const float fx = mx / size_factor;
	const float fy = my / size_factor;

	pangle = addmodpi( pangle, (0.00937) + (angle/256) );
	angle = cosf(pangle) * M_PI;
	sx = addmodpi( sx, 0.00673 );
	sy = addmodpi( sy, 0.00437 );
	tx = addmodpi( tx, 0.0239/mx );
	ty = addmodpi( ty, 0.0293/my );
	cx = addmodpi( cx, 0.00197 );
	cy = addmodpi( cy, 0.00227 );
	rcx = (mx/2) + (sinf(cx) * mx);
	rcy = (my/2) + (sinf(cy) * my);
	basecol = addmod( basecol, 1.0, 0.007 );

	matrix rotate = {
	  .v1_1 = cosf(angle),
	  .v1_2 = -sinf(angle),
	  .v2_1 = sinf(angle),
	  .v2_2 = cosf(angle),
	};
	matrix scale = {
	  .v1_1 = (sinf(sx) + 0.35)/fx,
	  .v1_2 = 0,
	  .v2_1 = 0,
	  .v2_2 = (cosf(sy) + 0.35)/fy,
	};
	vector translate = {
	  .x = sinf(tx) * mx,
	  .y = sinf(ty) * my,
	};

	int x;
	int y;
	for (y = 0; y < my; ++y)
		for (x = 0; x < mx; ++x) {
			vector c = vadd(vmmult(mmult(rotate, scale), (vector) { .x = x-rcx, .y = y-rcy }), translate);
			RGB col = HSV2RGB(HSV((basecol+sinecircle3D(c.x, c.y))*255, 255, 255));
			matrix_set(x, y, &col);
		};

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

int deinit() {
	return 0;
}
