// Starfield Simulation 1

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 2

static int modno;
static ulong frame;
static ulong nexttick;

#define INVERSE_STAR_DENSITY 100;

static const float z_speed = 0.01;
static const float roll_speed = 0.00;
static const float pitch_speed = 0.0;

typedef struct star_t {
    float fx;
    float fy;
    float fz;
} star_t;


static int no_of_stars ;
static star_t * stars = 0;

int init(int moduleno, char* argstr) {
    int mx = matrix_getx();
    int my = matrix_gety();
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
        star->fz = 0.0+r*1.0/128.0;
        r = rr & 0xff; rr >>= 8;
        star->fx = ((r-128)*1.0/128.0)/star->fz;
        r = rr & 0xff; rr >>= 8;
        star->fy = ((r-128)*1.0/128.0)/star->fz;
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
    star->fz += z_speed;
    if (roll_speed > 0){
        float x = star->fx;
        float y = star->fy;
        float c = cos(roll_speed);
        float s = sin(roll_speed);
        star->fx =  c*x + s*y;
        star->fy = -s*x + c*y;
    }
}

int draw(int argc, char* argv[]) {
    int mx = matrix_getx();
    int my = matrix_gety();
    matrix_clear();

    for (star_t * star=stars;star < stars + no_of_stars; star++){
        int px = (int)((star->fx * star->fz)*(mx/2))+mx/2;
        int py = (int)((star->fy * star->fz)*(my/2))+my/2;
        apply_transformation(star);
        if (px < 0 || px >= mx || py < 0 || py >= my){
            randomize_star(star);
            continue;
        }
        int brightness = 255;
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
