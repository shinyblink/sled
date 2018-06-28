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
static ulong frame;
static ulong nexttick;


int * data;
int sorted;
int boring;
int dir;
int second_stage;

// SETTINGS
const int color_range = 700;
const int swaps_per_frame = 500;
const int boring_threshold = 10;
const int soft_boring_threshold = 50;



RGB colorwheel(int angle){
    //angle = angle % 1536;
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

void fill_data(){
    int mx = matrix_getx();
    int my = matrix_gety();
    int color_offset = randn(1536);
    for (int i=0;i<mx;i++){
        for (int j=0;j<my;j++){
            data[i+j*mx] = randn(color_range) + color_offset;
        }
    }
}

void scmp(int * a, int * b){
    int t;
    if (*a < *b){
        t=*a;
        *a=*b;
        *b=t;
        boring++;
    }
}

void swapper(int * a, int * b, int * c, int * d){
    switch(dir){
        case 1:
            scmp(d,a);
            scmp(c,b);
            scmp(d,c);
            scmp(b,a);
            scmp(d,a);
            scmp(c,b);
            break;
        default:
            scmp(a,d);
            scmp(c,b);
            scmp(a,c);
    }
    //if (randn(4) & 1) scmp(b,c); else scmp(c,b);
}

void sort_data(){
    int mx = matrix_getx();
    int my = matrix_gety();
    for (int swaps=0;swaps<swaps_per_frame;swaps++){
        // try diagonal swap
        int size = mx*my;
        //int r = randn(mx*(my-1)-1);
        int x = randn(mx-2);
        int y = randn(my-2);
        //int r = randn(mx-1) + randn(my-1)*mx;
        int r = x + y*mx;
        int * p = data+r;
        if (p+mx+1 > data + mx*my){
            continue;
        }
        swapper(p,p+1,p+mx,p+mx+1);
    }
    if (boring < soft_boring_threshold){
        dir = 1;
        second_stage = frame+frame/4;
    }
    if (dir == 1){
        if (! --second_stage) reset();
    }

}


int init(int moduleno, char* argstr) {
    int mx = matrix_getx();
    int my = matrix_gety();
    data = malloc(sizeof(int) * mx * my);
	modno = moduleno;
	frame = 0;
	return 0;
}


void reset(void) {
    fill_data();
	nexttick = udate();
	matrix_clear();
	frame = 0;
    dir = 0;
    second_stage=0;
}

int draw(int argc, char* argv[]) {
    int mx = matrix_getx();
    int my = matrix_gety();

    boring = 0;
    sort_data();

    for (int i=0;i<mx;i++){
        for (int j=0;j<mx;j++){
            matrix_set(i,j,colorwheel(data[i+mx*j]));
        }
    }
    if (boring < 0){
        reset();
        return 1;
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
    free(data);
	return 0;
}
