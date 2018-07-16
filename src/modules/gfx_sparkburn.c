// Simple projectile/ball animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

static int modno;
static ulong nexttick;

static int mx,my;

static int * field;

static int rmax;
static int rmin;
static int threshold;

static char interesting = 0;

void reset(void) {
    rmax = -1000;
    rmin = 1000;
    for (int * p=field;p<field+mx*my;p++){
        int rr = rand();
        int r = 0;
        for (int i=0;i<8;i++){
            r += (rr>>i*4)&0xf;
        }
        //r /= 2;
        rmax = (rmax<r)?r:rmax;
        rmin = (rmin>r)?r:rmin;
        *p = 1+r;
    }
    for (int * p=field;p<field+mx*my;p++){
        *p -= (rmin);
    }
    threshold = (rmax-rmin);
	nexttick = udate();
	matrix_clear();
    interesting = 1;
}

void calc(){
    interesting = 0;
    if (threshold) threshold--;
    
    for (int i=0;i<mx;i++){
        for (int j=0;j<mx;j++){
            int * px = (field+i+j*mx);
            if (*px == 255){
                *px = 0;
                interesting = 1;
            }
            else if (*px >= threshold){

                *px *= 1.1;
                if (*px >= 256) {
                    *px = 255;
                }
                interesting = 1;
            }
            else if (*px > 0){
                *px += 1;
                interesting = 1;
            }
        }
    }

}

int draw(int argc, char* argv[]) {
    calc();
    for (int i=0;i<mx;i++){
        for (int j=0;j<mx;j++){
            char px = *(field+i+j*mx);
            matrix_set(i,j,RGB(px,px,px));
        }
    }

	matrix_render();
    if (interesting == 0){
        return 1;
    }

	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    field = malloc(mx*my*sizeof(int));
	modno = moduleno;
	return 0;
}

int deinit() {
    free(field);
	return 0;
}
