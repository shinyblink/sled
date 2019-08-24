/*
-  * affinematrix port, much donated by @orithena from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/XAffineFields.ino
+  * This is basically a slightly modified version of the affinematrix port,
+  * much donated by @orithena from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/XAffineFields.ino
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
#include <perf.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

/*** management variables ***/

static int modno;
static int frame = 0;
static oscore_time nexttick;

/*** matrix info (initialized in init()) ***/

static int mx, my;		// matrix size
static int mx2, my2;		// matrix half size
static float outputscale;	// matrix output scale factor

/*** base effect coefficients. This is where you want to play around. ***/

// how many run variables?
#define runvar_count 16

// run variable increments per frame
static float runinc[runvar_count] = {
  0.00123,   0.00651,  0.000471,  0.000973,
  0.000223,  0.000751,  0.000879,  0.000443,
  0.000373,  0.000459,  0.000321,  0.000247,
//   0.01273,  0.01459,  0.000321,  0.000247,
  0.000923,  0.00253,  0.00173,  0.000613
};

// variable overflow limits (basically the modulus of the ring the run variable runs in)
static float runmod[runvar_count] = {
  1.0,      2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI,
  2*M_PI,     2*M_PI,     2*M_PI,     2*M_PI
};

// the actual run variables
static float runvar[runvar_count] = {
  0,        0,        0,        0,
  0,        0,        0,        0,
  0,        0,        0,        0,
  0,        0,        0,        0
};

/* helper function: add on a ring
 */
static inline float addmod(float x, float mod, float delta) {
	x = fmodf(x + delta, mod);
	x += x<0 ? mod : 0;
	return x;
}

/*** module init ***/

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();
	if (mx < 2)
		return 1;
	if (my < 2)
		return 1;
	mx2 = mx/2;
	my2 = my/2;
	
	// scaling function thanks to @BenBE1987 on Twitter: https://twitter.com/BenBE1987/status/1003787341926985728
	outputscale = 2.0 * pow(2, -( (log2f(mx) - 3) + (log2f(mx) < 7 ? 0.5 : 0) * (7 - log2f(mx))));
	printf("(output scale for width=%d: %f) ", mx, outputscale);

	modno = moduleno;
	oscore_time d = udate();
	for( int i = 0; i < runvar_count; i++ ) {
		runvar[i] = addmod(runvar[i], runmod[i], ((d>>(i/2)) & 0x00FF) / (255/M_PI));
	}
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}


/* The "canvas" function.
 */
static inline float sinestuff1(float x, float y, float v0, float v1) {
  return ( cosf(v1+x) * sinf(v1+y) * cosf(v0 + sqrtf(x*x + y*y)) );
}
static inline float sinestuff2(float x, float y, float v0, float v1) {
  return sinf(( (cosf(v1+x)+sinf(x*1.731)) * (sinf(v1+y)+cosf(y*1.673)) + cosf(v0 + sqrtf(x*x + y*y)) )/2);
}


/* increment all run variables while taking care of overflow
 */
static void increment_runvars(void) {
  for( int i = 0; i < runvar_count; i++ ) {
    runvar[i] = addmod(runvar[i], runmod[i], runinc[i]);
  }
}

/* helper function: returns the absolute value of a float
 */
static inline float _abs(float x) {
  return x < 0 ? -x : x;
}

/* helper function: returns the minimum of two ints
 */
static inline int _min(int x, int y) {
	return x>y ? y : x;
}

static inline float crossfadef(float amount, float v0, float v1) {
	return ((1.0-amount) * v0) + (amount * v1);
}
static inline byte crossfadeb(float amount, byte v0, byte v1) {
	return (byte)((1.0-amount) * v0) + (byte)(amount * v1);
}
static inline int crossfadei(float amount, int v0, int v1) {
	return (int)((1.0-amount) * v0) + (int)(amount * v1);
}

/* central drawing function
 */
int draw(int _modno, int argc, char* argv[]) {
	nexttick = udate() + FRAMETIME;
	perf_start(modno);

	// compose transformation matrix out of 9 input matrices 
	// which are calculated from some of the run variables
	matrix3_3 m = composem3( 9,
		rotation3(cosf(runvar[12]) * M_PI),
		translation3(cosf(runvar[2])*mx*0.125, sinf(runvar[3])*my*0.125),
		scale3(outputscale, outputscale),
		rotation3(runvar[13]),
		translation3(sinf(runvar[4])*mx*0.25, cosf(runvar[5])*my*0.25),
		rotation3(sin(runvar[14]) * M_PI),
		translation3(sinf(runvar[6])*mx*0.125, cosf(runvar[7])*my*0.125),
		rotation3(runvar[15]),
// 		scale3(0.25+sinf(runvar[8])/4.0, 0.25+cosf(runvar[9])/4.0)
		scale3(0.6+sinf(runvar[8])/4.0, 0.6+cosf(runvar[9])/4.0)
	);

	// pre-calculate some variables outside the loop
	float pc1 = cosf(runvar[1]);
	float pc121 = 0.125+((pc1/4) * sinf(runvar[11]));
	float pc01 = runvar[0] + pc1;
	float pc10 = (mx2*sinf(runvar[10]));
	
	float fader = (cosf(runvar[1]) + 1.0) / 2.0;

	perf_print(modno, "Composition");

	// actual pixel loop
	for( int x = 0; x < mx; x++ ) {
		vec2 kernel_x = multm3v2_partx(m, x-(mx2));
		for( int y = 0; y < my; y++ ) {

			// transform x,y coordinates by the pre-composed matrix
			vec2 v = multm3v2_partxy(m, kernel_x, y-(my2));

			// calculate sine curve point
			float sc1 = sinestuff1(v.x, v.y, pc10, runvar[11]);
			float sc2 = sinestuff2(v.x, v.y, pc10, runvar[11]);
			float sc = crossfadef(fader, sc1, sc2);

			// add changing base hue to sine curve point
			float hue1 = pc01 + (sc * 0.5);
			float hue2 = ((x-mx2 + y-my2)*5) +  pc01 + (sc * pc121) + sinf((v.x+v.y)/2);
			float hue = crossfadef(fader, hue1, hue2);

			// calculate value of HSV float value
			int i_val2 = (int)(_abs(sc+0.125)*320);
			byte b_val2 = _min(255, i_val2);

			// calculate byte value of HSV float hue ( [0.0..1.0] -> [0..255], overflows are intended! )
			int i_hue1 = (int)(hue*256);
			int i_hue2 = (int)(hue*256) + i_val2;
			byte b_hue = crossfadei(fader, i_hue1, i_hue2) & 0xFF;

			// convert HSV to RGB
			RGB color = HSV2RGB(HSV( b_hue, 255, b_val2 ));

			// set pixel in matrix framebuffer
			matrix_set(x,y, color);
		}
	}

	perf_print(modno, "Drawing");

	// render it out
	matrix_render();

	perf_print(modno, "Rendering");
	
	increment_runvars();

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
