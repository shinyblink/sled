// Simple projectile/ball animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>

#define FPS 260
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

static int modno;
static ulong frame;
static ulong nexttick;

RGB colorwheel(int angle){
    angle = angle % 1536;
    int t = (angle / 256)%6;
    int v = angle % 256;
    switch (t){
    case 0: return RGB(255,v,0);
    case 1: return RGB(255-v,255,0);
    case 2: return RGB(0,255,v);
    case 3: return RGB(0,255-v,255);
    case 4: return RGB(v,0,255);
    case 5: return RGB(255,0,255-v);
    }
}

RGB randcolor(){
    return colorwheel(randn(1536));
}

int init(int moduleno, char* argstr) {
	modno = moduleno;
	frame = 0;
	return 0;
}


void reset(void) {
	nexttick = udate();
	matrix_clear();
	frame = 0;
}

int draw(int argc, char* argv[]) {
    int mx = matrix_getx();
    int my = matrix_gety();
    for (int i=0;i<mx;i++){
        for (int j=0;j<mx;j++){
            matrix_set(i,j,colorwheel(frame));
        }
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
	return 0;
}
