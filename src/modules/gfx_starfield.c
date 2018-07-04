// Starfield Simulation 1

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 2

static int modno;
static ulong frame;
static ulong nexttick;

#define INVERSE_STAR_DENSITY 100;

static const float z_speed = 0.03;

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
        star->fx = ((r-128)*1.0/256.0);
        r = rr & 0xff; rr >>= 8;
        star->fy = ((r-128)*1.0/256.0);
        r = rr & 0xff; rr >>= 8;
        star->fz = 1.0+r*1.0/256.0;
}

void reset(void) {
    for (star_t * star=stars;star < stars + no_of_stars; star++){
        randomize_star(star);
    }
	nexttick = udate();
	matrix_clear();
	frame = 0;
}

int draw(int argc, char* argv[]) {
    int mx = matrix_getx();
    int my = matrix_gety();
    matrix_clear();

    for (star_t * star=stars;star < stars + no_of_stars; star++){
        int px = (int)((star->fx * star->fz)*(mx/2))+mx/2;
        int py = (int)((star->fy * star->fz)*(my/2))+my/2;
        star->fz += z_speed;
        if (px < 0 || px >= mx || py < 0 || py >= my){
            randomize_star(star);
            continue;
        }
        matrix_set(px,py,RGB(255,255,255));
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
