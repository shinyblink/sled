// A fire-like animation.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <mathey.h>

#define FRAMES 255
#define FRAMETIME ((TIME_LONG * T_SECOND) / 255)

static int modno;
static int pos;
static int frame = 0;
static oscore_time nexttick;

static int fire_bufsize;
static byte* fire_buffer[2];
static int mx, my;
static int fx, fy, fo, current_fire;
static RGB fire_palette[256];

static inline int ringmod(int x, int m) {
	return (x % m) + (x<0?m:0);
}
static inline int clamp(int l, int x, int u) {
	return (x<l?l:(x>u?u:x));
}

#define CUR current_fire
#define LAST (current_fire^1)
#define LFXY(_X,_Y) fire_buffer[LAST][ ringmod(_X, mx) + (clamp(0, _Y, mx)*mx) ]
#define CFXY(_X,_Y) fire_buffer[CUR][ ringmod(_X, mx) + (clamp(0, _Y, mx)*mx) ]

static RGB fire_palette_func(byte shade) {
 	double r = 1-cos(((shade/255.0)*M_PI)/2);
 	double g = ((1-cos(((shade/255.0)*6*M_PI)/2))/2) * (r*r*0.5);
 	double b = (1-cos((bmin(shade-128,0)/128.0)*0.5*M_PI)) * r;
 	return RGB(r*255, g*255, b*255);
}

static void fire_addcoal() {
	int i1 = randn(fx);
	int i2 = i1 + randn(fx/6) + (fx/8);
	for( int x = i1; x <= i2; x++ ) {
		CFXY(x, 0) = CFXY(x, 1) = LFXY(x, 0) = LFXY(x, 1) = bmin(CFXY(x,0), 63 + (192 * ( sin(((x-i1)*(M_PI))/(i2-i1)) )));
	}
}

static void fire_cooldown() {
	int d = randn(7) + 3;
	for( int x = 0; x < fx; x++ ) {
		for( int y = 0; y < 2; y++ ) {
			if( LFXY(x,y) > 128 ) {
				CFXY(x,y) = LFXY(x,y) = CFXY(x,y) - d;
			} else {
				CFXY(x,y) = LFXY(x,y) = 128;
			}
		}
	}
	for( int x = 0; x < fx; x++ ) {
		for( int y = 2; y < fy; y++ ) {
			if( CFXY(x,y) > 3 ) {
				CFXY(x, y) -= randn(4);
			} else if( CFXY(x, y) > 0) {
				CFXY(x, y) -= 1;
			}
		}
	}
}

static void fire_generation() {
	fire_addcoal();
	fire_cooldown();
	CUR = LAST;
	for( int x = 0; x < fx; x++ ) {
		for( int y = 1; y < fy-1; y++ ) {
			CFXY(x,y+1) = (LFXY(x,y-1) + LFXY(x,y+1) + LFXY(x-1,y) + LFXY(x+1,y)) / 4;
		}
	}
}

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 8)
		return 1;
	modno = moduleno;
	mx = matrix_getx();
	my = matrix_gety();
	fo = 2;
	fx = mx;
	fy = my + fo;
	fire_bufsize = fx*fy;
	fire_buffer[0] = malloc(fire_bufsize);
	fire_buffer[1] = malloc(fire_bufsize);
	for( int i = 0; i <=255; i++ ) {
		fire_palette[i] = fire_palette_func(i);
	}
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	nexttick = udate() + (1000000/30);
	int x;
	int y;
	fire_generation();
	for (y = 0; y < my; ++y) {
		for (x = 0; x < mx; ++x) {
			RGB color = fire_palette[CFXY(x,y+2)];
			matrix_set(x, my-y-1, color);
		}
	}

	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		pos = 0;
		return 1;
	}
	frame++;
	pos++;;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(fire_buffer[0]);
	free(fire_buffer[1]);
}
