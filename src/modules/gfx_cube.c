// NOTE: Right now this is in early stages. Code should get refactors, particularly anything... matrixy - 20kdc
// Look, 3D stuff is simple, really! Let me show you, with a song!

// First we do our links dynamic,

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <stdio.h>
#include <random.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <graphics.h>

// Then a bunch of defines,
#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_SHORT * FPS)

// then we check our statics,

// Since 3D vectors aren't in mathey we just invent our own
#define LINES 12

static float points[LINES * 6] = {
	// Firstly, do the Z-1 4 rods
	-1, -1, -1, 1, -1, -1,
	-1, -1, -1, -1, 1, -1,
	1, -1, -1, 1, 1, -1,
	1, 1, -1, -1, 1, -1,

	-1, -1, 1, 1, -1, 1,
	-1, -1, 1, -1, 1, 1,
	1, -1, 1, 1, 1, 1,
	1, 1, 1, -1, 1, 1,

	// connectors
	-1, -1, -1, -1, -1, 1,
	-1, 1, -1, -1, 1, 1,
	1, -1, -1, 1, -1, 1,
	1, 1, -1, 1, 1, 1,
};

// This gets updated at runtime!!!
static float matrix[] = {
	1, 0, 0, // X Basis
	0, 1, 0, // Y Basis
	0, 0, 1, // Z Basis
	0, 0, 3, // Bias (cannot allow vertices to go to Z 0)
};

// then some functions so fine,

// The full transform pipeline we're using here.
// matrix is the modelview matrix, projection gets done here.
static float * transform3d(float * x, float * y, float * src, float aspectx, float aspecty, float mw, float mh) {
	float sx, sy, sz, dz;
	sx = *(src++);
	sy = *(src++);
	sz = *(src++);
	*x = (matrix[0] * sx) + (matrix[3] * sy) + (matrix[6] * sz) + matrix[9];
	*y = (matrix[1] * sx) + (matrix[4] * sy) + (matrix[7] * sz) + matrix[10];
	dz = (matrix[2] * sx) + (matrix[5] * sy) + (matrix[8] * sz) + matrix[11];
	dz = fmaxf(0.01f, dz);
	// This could be integrated into the matrix, but that's complicated.
	if (aspectx > aspecty) {
		*x /= aspectx;
	} else {
		*y /= aspecty;
	}
	*x /= dz;
	*y /= dz;
	*x = ((*x) + 1) / 2;
	*y = ((*y) + 1) / 2;
	*x *= mw;
	*y *= mh;
	return src;
}

// Now our major automatics,
static int moduleno, framecount;
static oscore_time nexttick;

// And then the main functions sublime!

int init(int _moduleno, char* argstr) {
	if (matrix_getx() < 16 || matrix_gety() < 16)
		return 1;

	moduleno = _moduleno;
	framecount = 0;
	nexttick = 0;
	return 0;
}

void reset(int _modno) {
	framecount = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	// Prepare aspect/size variable stuff
	float aspectx, aspecty, mw, mh;
	aspectx = ((float) matrix_getx()) / matrix_gety();
	aspecty = ((float) matrix_gety()) / matrix_getx();
	mw = matrix_getx();
	mh = matrix_gety();

	// X basis
	matrix[0] = cosf(((float) framecount) / 100);
	matrix[2] = sinf(((float) framecount) / 100);
	// Z basis
	matrix[6] = sinf(((float) -framecount) / 100);
	matrix[8] = cosf(((float) -framecount) / 100);

	RGB white = RGB(255, 255, 255);
	matrix_clear();
	// execute lines
	float * lp = points;
	for (int i = 0; i < LINES; i++) {
		float sx, sy;
		float dx, dy;
		lp = transform3d(&sx, &sy, lp, aspectx, aspecty, mw, mh);
		lp = transform3d(&dx, &dy, lp, aspectx, aspecty, mw, mh);
		graphics_drawline((int) sx, (int) sy, (int) dx, (int) dy, white);
	}
	matrix_render();
	// Timing logic
	if (framecount == 0) {
		nexttick = udate();
	} else if (framecount >= FRAMES) {
		framecount = 0;
		return 1;
	}
	framecount++;
	nexttick += FRAMETIME;
	timer_add(nexttick, moduleno, 0, NULL);
	return 0;
}

void deinit(int _modno) {}
