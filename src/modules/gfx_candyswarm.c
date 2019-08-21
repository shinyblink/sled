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
#include <stdlib.h>
#include <random.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

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
	float pos_x;
	float pos_y;
	float vel_x;
	float vel_y;
    int xa;
    int ya;
} ball;

static int numballs;
static ball* balls;


// mode flags and settings
typedef char bool;
static const bool true = 1;
static const bool false = 0;
static bool USE_COLOR;
static bool USE_ADDITIVE_COLOR;
static bool USE_PERTURBATION;
static bool USE_POTENTIAL;
static bool USE_PULSATION;
static bool USE_DAMPENING;
static bool USE_ROTATION;
static bool USE_PULSATION_DAMPENING;
static bool USE_POTENTIAL_DAMPENING;
static bool USE_POTENTIAL_BULK;
static bool USE_TRAILS;
static int TRAIL_LENGTH;
static RGB MONOCHROMATIC_COLOR;
static float DAMPENING_CONSTANT;
static float POTENTIAL_SIZE;
static float POTENTIAL_ECCENTRICITY;




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
	oscore_time d = udate();
	for( int i = 0; i < runvar_count; i++ ) {
		runvar[i] = addmod(runvar[i], runmod[i], ((d>>(i/2)) & 0x00FF) / (255/M_PI));
	}
    
    numballs = (mx/32)*(mx+my);
    numballs = mx*my/5;
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
        b->pos_x += (float) randn(numballs)*1.0/numballs;
        b->pos_y += (float) randn(numballs)*1.0/numballs;
        b->vel_x = 0;
        b->vel_y = 0;
        b->xa = (int) b->pos_x;
        b->ya = (int) b->pos_y;
    }

    // mode config
    USE_COLOR = true;
    USE_ADDITIVE_COLOR = false;
    USE_POTENTIAL = true;
    USE_PERTURBATION = true;
    USE_PULSATION = false;
    USE_DAMPENING = false;
    USE_ROTATION = false;
    USE_POTENTIAL_DAMPENING = false;
    USE_POTENTIAL_BULK = false;
    USE_PULSATION_DAMPENING = false;
    USE_TRAILS = true;
    TRAIL_LENGTH = 230;
    DAMPENING_CONSTANT = 0.98;
    MONOCHROMATIC_COLOR = RGB(255,255,255);
    POTENTIAL_SIZE = 1;
    POTENTIAL_ECCENTRICITY = 0.8;
    int r = randn(18);
    int i = 1;
    //r = 18;
    if (r == i++){ // 1
        USE_TRAILS = false;
    } else if (r == i++){ // 2
        USE_PERTURBATION = false;
    } else if (r == i++){ // 3
        USE_ADDITIVE_COLOR = true;
    } else if (r == i++){ // 4
        USE_PERTURBATION = false;
        USE_COLOR = false;
    } else if (r == i++){ // 5
        USE_PERTURBATION = false;
        USE_COLOR = false;
        USE_TRAILS = false;
        USE_ADDITIVE_COLOR = true;
    } else if (r == i++){ // 6
        USE_PULSATION = true;
        USE_PULSATION_DAMPENING = true;
        POTENTIAL_SIZE = 0.8;
        DAMPENING_CONSTANT = 0.99;
    } else if (r == i++){ // 7
        USE_ROTATION = true;
        USE_PULSATION = true;
        USE_PULSATION_DAMPENING = true;
        DAMPENING_CONSTANT = 0.99;
        POTENTIAL_SIZE = 0.6;
    } else if (r == i++){ // 8
        USE_PULSATION = true;
        USE_POTENTIAL_DAMPENING = true;
        POTENTIAL_SIZE = 0.5;
    } else if (r == i++){ // 9
        USE_PULSATION = true;
        USE_POTENTIAL_DAMPENING = true;
        USE_ADDITIVE_COLOR = true;
        POTENTIAL_SIZE = 0.5;
    } else if (r == i++){ // 10
        USE_DAMPENING = true;
        POTENTIAL_SIZE = 3;
    } else if (r == i++){ // 11
        USE_DAMPENING = true;
        POTENTIAL_SIZE = 3;
        USE_ADDITIVE_COLOR = true;
    } else if (r == i++){ // 12
        POTENTIAL_ECCENTRICITY = 0.03;
        POTENTIAL_SIZE = 0.8;
        USE_POTENTIAL_DAMPENING = true;
        TRAIL_LENGTH = 240;
        USE_COLOR = false;
        USE_ADDITIVE_COLOR = true;
        MONOCHROMATIC_COLOR = RGB(40,40,40);
    } else if (r == i++){ // 13
        POTENTIAL_ECCENTRICITY = 0.03;
        POTENTIAL_SIZE = 0.8;
        USE_POTENTIAL_DAMPENING = true;
        USE_PERTURBATION = false;
        TRAIL_LENGTH = 240;
        USE_COLOR = false;
        USE_ADDITIVE_COLOR = true;
        MONOCHROMATIC_COLOR = RGB(40,40,40);
    } else if (r == i++){ // 14
        POTENTIAL_ECCENTRICITY = 0.03;
        POTENTIAL_SIZE = 0.8;
        //USE_DAMPENING = true;
        USE_PERTURBATION = false;
        //USE_TRAILS = false;
        USE_COLOR = false;
        USE_PULSATION = true;
        USE_ADDITIVE_COLOR = true;
    } else if (r == i++){ // 15
        POTENTIAL_ECCENTRICITY = 0.03;
        POTENTIAL_SIZE = 0.8;
        USE_DAMPENING = true;
        USE_POTENTIAL_DAMPENING = true;
        USE_PERTURBATION = false;
        USE_TRAILS = false;
        USE_COLOR = true;
        USE_PULSATION = true;
        USE_ADDITIVE_COLOR = true;
    } else if (r == i++){ // 16
        POTENTIAL_ECCENTRICITY = 0.8;
        POTENTIAL_SIZE = 0.8;
        USE_POTENTIAL= true;
        USE_POTENTIAL_BULK = true;
        USE_DAMPENING = true;
        USE_POTENTIAL_DAMPENING = true;
        USE_PERTURBATION = true;
        TRAIL_LENGTH = 240;
        USE_COLOR = false;
        USE_ADDITIVE_COLOR = true;
        MONOCHROMATIC_COLOR = RGB(40,40,40);
    } else if (r == i++){ // 17
        POTENTIAL_ECCENTRICITY = 0.8;
        POTENTIAL_SIZE = 0.8;
        USE_POTENTIAL= true;
        USE_POTENTIAL_BULK = true;
        USE_DAMPENING = true;
        USE_POTENTIAL_DAMPENING = true;
        USE_PERTURBATION = false;
        TRAIL_LENGTH = 240;
        USE_COLOR = false;
        USE_ADDITIVE_COLOR = true;
        MONOCHROMATIC_COLOR = RGB(40,40,40);
    } else if (r == i++){ // 18
        POTENTIAL_ECCENTRICITY = 0.2;
        POTENTIAL_SIZE = 0.8;
        USE_POTENTIAL= true;
        USE_POTENTIAL_BULK = true;
        USE_DAMPENING = false;
        USE_POTENTIAL_DAMPENING = true;
        USE_PERTURBATION = false;
        TRAIL_LENGTH = 240;
        USE_COLOR = true;
        USE_ADDITIVE_COLOR = true;
        MONOCHROMATIC_COLOR = RGB(40,40,40);
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
    float slow_phase = cosf(frame/100.0);
    slow_phase *= slow_phase;

	perf_print(modno, "Composition");
    if (USE_TRAILS){
        for (int x = 0;x < matrix_getx();x++){
            for (int y = 0;y < matrix_gety();y++){
                RGB color;
                color = matrix_get(x,y);
                color.red = color.red * TRAIL_LENGTH/256;
                color.green = color.green * TRAIL_LENGTH/256;
                color.blue = color.blue * TRAIL_LENGTH/256;
                matrix_set(x,y,color);
            }
        }
    } else {
        for(ball * b = balls;b < balls+numballs;b++) {
            int x = (int) b->pos_x;
            int y = (int) b->pos_y;
            if (x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety())
                matrix_set(x,y, RGB(0,0,0));
        }
    }


	// actual pixel loop
	for(ball * b = balls;b < balls+numballs;b++) {
        int x = (int) b->pos_x;
        int y = (int) b->pos_y;
		vec2 kernel_x = multm3v2_partx(m, x-(mx2));
        vec2 v = multm3v2_partxy(m, kernel_x, y-(my2));


        float scxp = sinestuff(v.x+1, v.y, pc10, runvar[11]);
        float scxn = sinestuff(v.x-1, v.y, pc10, runvar[11]);
        float scyp = sinestuff(v.x, v.y+1, pc10, runvar[11]);
        float scyn = sinestuff(v.x, v.y-1, pc10, runvar[11]);

        if (USE_PERTURBATION) {
            b->vel_x += ((scxp - scxn)/2)/100;
            b->vel_y += ((scyp - scyn)/2)/100;
        }


        float xx = (x - mx/2)/(1.0*mx/2);
        float yy = (y - my/2)/(1.0*my/2);
        float rr = hypotf(xx,yy);

        rr /= POTENTIAL_SIZE;

        if (USE_PULSATION){
            rr /= 0.8+slow_phase;
        }

        if (USE_POTENTIAL){
            b->vel_x += -rr*rr*rr*rr*xx/100;
            b->vel_y += -rr*rr*rr*rr*yy/(100*POTENTIAL_ECCENTRICITY);
            if (USE_POTENTIAL_BULK){
                b->vel_x += rr*rr*xx/100;
                b->vel_y += rr*rr*yy/(100*POTENTIAL_ECCENTRICITY);
            }

        }
        
        if (USE_ROTATION){
            b->vel_x += yy/100;
            b->vel_y += -xx/100;
        }

        if (USE_DAMPENING||USE_POTENTIAL_DAMPENING||USE_PULSATION_DAMPENING){
            if (USE_POTENTIAL_DAMPENING){
                float factor = rr*rr*rr*rr/1000;
                b->vel_x *= 1-(1-DAMPENING_CONSTANT)*factor;
                b->vel_y *= 1-(1-DAMPENING_CONSTANT)*factor;
            }else if (USE_PULSATION_DAMPENING) {
                b->vel_x *= 1-(1-DAMPENING_CONSTANT)*slow_phase;
                b->vel_y *= 1-(1-DAMPENING_CONSTANT)*slow_phase;
            } else {
                b->vel_x *= DAMPENING_CONSTANT;
                b->vel_y *= DAMPENING_CONSTANT;
            }
        }



        b->pos_x += b->vel_x;
        b->pos_y += b->vel_y;
	    
        float hue = ((b->xa-mx2 + b->ya-my2)*5) +  pc01 + (scxp * pc121) + sinf((v.x+v.y)/2);
        int i_val = (int)(_abs(scxp+0.125)*240)+80;
        //int i_val = 255;
        byte b_val = _min(255, i_val);
        byte b_hue = ((int)(hue*256) + i_val) & 0xFF;
        RGB color = HSV2RGB(HSV( b_hue, 255, b_val ));
        b->color = color;

        x = (int) b->pos_x;
        y = (int) b->pos_y;
        if (x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety()) {
            RGB this_color = MONOCHROMATIC_COLOR;
            if (USE_COLOR) this_color = b->color;
            if (USE_ADDITIVE_COLOR){
                RGB c = matrix_get(x,y);
                c.red = _min(c.red +this_color.red/5,255);
                c.green = _min(c.green + this_color.green/5,255);
                c.blue = _min(c.blue + this_color.blue/5,255);
                matrix_set(x,y,c);
            } else {
                matrix_set(x,y, this_color);
            }
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

void deinit() {
    free(balls);
}
