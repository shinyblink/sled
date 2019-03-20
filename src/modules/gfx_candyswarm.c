/*
 * This a slight modification of candyflow which itself is
 * basically a slightly modified version of the affinematrix port,
 * much donated by @orithena from https://github.com/orithena/Arduino-LED-experiments/blob/master/MatrixDemo/XAffineFields.ino
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
#define FRAMES (TIME_LONG * FPS)

/*** management variables ***/

static int modno;
static int frame = 0;
static ulong nexttick;

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
  0.01273,  0.01459,  0.000321,  0.000247,
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

/////////// BALL STUFF ///////////
typedef struct ball {
	RGB color;
    float a;
    float b;
	float pos_x;
	float pos_y;
	float vel_x;
	float vel_y;
} ball;

static int numballs;
static ball* balls;




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
	outputscale = pow(2, -( (log2f(mx) - 3) + (log2f(mx) < 7 ? 0.5 : 0) * (7 - log2f(mx))));
	printf("(output scale for width=%d: %f) ", mx, outputscale);
	
	modno = moduleno;
	ulong d = udate();
	for( int i = 0; i < runvar_count; i++ ) {
		runvar[i] = addmod(runvar[i], runmod[i], ((d>>(i/2)) & 0x00FF) / (255/M_PI));
	}
    
    numballs = 16*(mx+my);
    balls = malloc(numballs*sizeof(ball));


	return 0;
}

static inline int _min(int,int);
void reset(void) {
    matrix_clear();
	nexttick = udate();
	frame = 0;
    for (ball * b = balls; b < balls + numballs; b++){
        b->pos_x = (float) randn(matrix_getx());
        b->pos_y = (float) randn(matrix_gety());
        b->vel_x = 0;
        b->vel_y = 0;
        b->a = (int)b->pos_x;
        b->b = (int)b->pos_y;
    }
}


/* The "canvas" function.
 */
static inline float sinestuff(float x, float y, float v0, float v1) {
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

/* central drawing function
 */
int draw(int argc, char* argv[]) {
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
		scale3(0.6+sinf(runvar[8])/4.0, 0.6+cosf(runvar[9])/4.0)
	);

	// pre-calculate some variables outside the loop
	float pc1 = cosf(runvar[1]);
	float pc121 = 0.125+((pc1/4) * sinf(runvar[11]));
	float pc01 = runvar[0] + pc1;
	float pc10 = (mx2*sinf(runvar[10]));

	perf_print(modno, "Composition");

	// actual pixel loop
	for(ball * b = balls;b < balls+numballs;b++) {

        int x = (int) b->pos_x;
        int y = (int) b->pos_y;
        if (x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety())
            matrix_set(x,y, RGB(0,0,0));
		vec2 kernel_x = multm3v2_partx(m, x-(mx2));
        vec2 v = multm3v2_partxy(m, kernel_x, y-(my2));

        float scxp = sinestuff(v.x+1, v.y, pc10, runvar[11]);
        float scxn = sinestuff(v.x-1, v.y, pc10, runvar[11]);
        float scyp = sinestuff(v.x, v.y+1, pc10, runvar[11]);
        float scyn = sinestuff(v.x, v.y-1, pc10, runvar[11]);
        b->vel_x += ((scxp - scxn)/2)/100;
        b->vel_y += ((scyp - scyn)/2)/100;
        float xx = (x - mx/2)/(1.0*mx/2);
        float yy = (y - my/2)/(1.0*my/2);
        float rr = hypotf(xx,yy);

        b->vel_x += -rr*rr*rr*rr*xx/100;
        b->vel_y += -rr*rr*rr*rr*yy/100;

        b->pos_x += b->vel_x;
        b->pos_y += b->vel_y;
	    
        float hue = ((b->b-mx2 + b->b-my2)*5) +  pc01 + (scxp * pc121) + sinf((v.x+v.y)/2);
        int i_val = (int)(_abs(scxp+0.125)*240)+80;
        //int i_val = 255;
        byte b_val = _min(255, i_val);
        byte b_hue = ((int)(hue*256) + i_val) & 0xFF;
        RGB color = HSV2RGB(HSV( b_hue, 255, b_val ));
        b->color = color;

        x = (int) b->pos_x;
        y = (int) b->pos_y;
        if (x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety())
            //matrix_set(x,y, RGB(255,255,255));
            matrix_set(x,y,b->color);
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

int deinit() {
	return 0;
}