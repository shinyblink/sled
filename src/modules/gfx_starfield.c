// Starfield Simulation 1

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 2

static int modno;
static ulong frame;
static ulong nexttick;
static int mx;
static int my;

#define INVERSE_STAR_DENSITY 100;

#define MOVEMENT_6DOF

static const float z_speed = 0.02;
#ifdef MOVEMENT_6DOF
static const float x_speed = 0.00;
static const float y_speed = 0.0;
static const float roll_speed = 0.00;
static const float pitch_speed = 0.00;
static const float yaw_speed = 0.00;
#endif 


typedef struct star_t {
    float fx;
    float fy;
    float fz;
    int brightness;
} star_t;


static int no_of_stars ;
static star_t * stars = 0;

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    no_of_stars = (mx*my)/INVERSE_STAR_DENSITY;
    stars = malloc(no_of_stars*sizeof(star_t));
	modno = moduleno;
	frame = 0;
	return 0;
}

void randomize_star(star_t * star){
        uint rr = rand();
        int r;
        r = rr & 0xff; rr >>= 8;
        star->fz = 1.0+r*1.0/64.;
        r = rr & 0xff; rr >>= 8;
        star->fx = ((r-128)*1.0/128.0*mx)*star->fz;
        r = rr & 0xff; rr >>= 8;
        star->fy = ((r-128)*1.0/128.0*my)*star->fz;
        r = rr & 0xff; rr >>= 8;
        star->brightness = 128 + r/2;
}

void rerandomize_star(star_t * star){

}

void reset(void) {
    for (star_t * star=stars;star < stars + no_of_stars; star++){
        randomize_star(star);
    }
	nexttick = udate();
	matrix_clear();
	frame = 0;
}

void apply_transformation(star_t * star){
    star->fz -= z_speed;
#ifdef MOVEMENT_6DOF
    star->fx -= x_speed;
    star->fy -= y_speed;
    if (roll_speed != 0.0){
        float x = star->fx;
        float y = star->fy;
        float c = cos(roll_speed);
        float s = sin(roll_speed);
        star->fx =  c*x + s*y;
        star->fy = -s*x + c*y;
    }
    if (pitch_speed != 0.0){
        float y = star->fy;
        float z = star->fz;
        if (isnormal(z)){
            float c = cos(pitch_speed);
            float s = sin(pitch_speed);
            star->fy =  c*y + s*z;
            star->fz = -s*y + c*z;
        }
    }
    if (yaw_speed != 0.0){
        float x = star->fx;
        float z = star->fz;
        if (isnormal(z)){
            float c = cos(yaw_speed);
            float s = sin(yaw_speed);
            star->fx =  c*x + s*z;
            star->fz = -s*x + c*z;
        }
    }
#endif

}

int draw(int argc, char* argv[]) {
    matrix_clear();
    int scale = (mx<my)?mx:my;

    for (star_t * star=stars;star < stars + no_of_stars; star++){
        int px = (int)((star->fx / star->fz))+mx/2;
        int py = (int)((star->fy / star->fz))+my/2;
        apply_transformation(star);
        if (px < 0 || px >= mx || py < 0 || py >= my){
            randomize_star(star);
            continue;
        }
        int brightness = star->brightness;
        matrix_set(px,py,RGB(brightness,brightness,brightness));
    }
    

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
    free(stars);
	return 0;
}
