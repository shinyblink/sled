// a kind of 2D Bubblesort
// The sorting Network for 2x2 px looks like:
// A B
// |X   
// C D
//
// This is applied to random points
//
//
//
// Copyright (c) 2019, Jonathan Cyriax Brast
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <timers.h>
#include <stdio.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS) * 10

// calculate and print timing information
#define SORT_TIMING
// Use bitmask to store where updates happened
// Seems faster without on an i5
//#define USE_BITMASK
static int modno;
static int frame;
static oscore_time nexttick;

static oscore_time t1;
static oscore_time t2;
static oscore_time t3;
static oscore_time td1_acc;
static oscore_time td2_acc;
static oscore_time timer_n;

static int * data;
static char * data_bitmask;
static int dir;
static int second_stage;
static int comparisons_hot;
static int comparisons_cold;
static int exit_flag;

// SETTINGS
static const int s_generator_step = 8;
static const int s_boring_percentage = 8;

// GENERATED SETTINGS
static int boring_percentage = 1;
static int color_range = 700;


static void own_reset();

static RGB colorwheel(int angle){
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
	default: return RGB(0,0,0);
	}
}

static void fill_data(){
	int mx = matrix_getx();
	int my = matrix_gety();
	int color_offset = randn(1536);
	for (int i=0;i<mx;i++){
		for (int j=0;j<my;j++){
			data[i+j*mx] = randn(color_range) + color_offset;
            matrix_set(i,j,colorwheel(data[i+mx*j]));
		}
	}
    for (int i=0;i<(mx*my+1)/8;i++) data_bitmask[i] = 0xff;
}

void generate_settings(){
    uint r = rand();
	dir = (r & 1) * 2;
    r >>= 1;
    switch ((r&3)){
        case 0:
        case 1:
        case 2:
            dir = 0;
            boring_percentage = s_boring_percentage;
            break;
        case 3:
            dir = 2;
            boring_percentage = 1;
            break;
    }
    r >>= 2;

    switch (r&7){
        case 0:
        case 1:
        case 2:
            color_range = 700;
            break;
        case 3:
            color_range = 5000;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            color_range = 1500;
            break;
    }
    r >>= 3;

	frame = 0;
	second_stage=0;
    exit_flag=0;
}

static void scmp(int * a, int * b){
	int t;
	if (*a < *b){
		t=*a;
		*a=*b;
		*b=t;
        comparisons_hot++;
	} else {
		comparisons_cold++;
    }
}

static void swapper(int * a, int * b, int * c, int * d){
	switch(dir){
        case 0:
		default:
			scmp(a,d);
			scmp(c,b);
			scmp(a,c);
            break;
		case 1:
			scmp(d,a);
			scmp(c,b);
			scmp(d,c);
			break;
        case 2:
            scmp(a,d);
            scmp(a,c);
            break;
        case 3:
			scmp(d,a);
			scmp(d,c);
            break;
        case 4:
            scmp(c,b);
            scmp(a,d);
            scmp(d,a);
            break;
	}
	//if (randn(4) & 1) scmp(b,c); else scmp(c,b);
}

static void sort_data(){
	int mx = matrix_getx();
	int my = matrix_gety();
    //printf("mx x my = %d x %d\n",mx,my);
    comparisons_hot = 0;
    comparisons_cold = 0;
    uint entropy = rand();

    // generate random points
    for (int i = 0;;){
            if (entropy == 0) entropy = rand();
            int step = entropy % s_generator_step;
            entropy /= s_generator_step;
            i += step;
            if (i%mx >= mx-1) i++;
            if (i + mx + 1 > (mx * my)) break;
            //printf(" %d = [%d] %d\n",i,i/8,i%8);
		    int * p = data+i;
		    swapper(p,p+1,p+mx,p+mx+1);
#ifdef USE_BITMASK
            int j;
            j = i; data_bitmask[j/8] |= 1<<(j%8);
            j = i+1; data_bitmask[j/8] |= 1<<(j%8);
            j = i+mx; data_bitmask[j/8] |= 1<<(j%8);
            j = i+mx+1; data_bitmask[j/8] |= 1<<(j%8);
#endif
    }


	if (comparisons_hot * 100 < boring_percentage * (comparisons_cold + comparisons_hot)){
        switch (dir){
            case 0:
                if (randn(3)){
                    dir = 1;
                } else {
                    dir = 4;
                }
                second_stage = frame+frame/3;
                break;
            case 1:
                exit_flag = 1;
                break;
            case 2:
                dir = 2;
                if (second_stage == 0) second_stage = frame/3;
                break;
            case 3:
                break;
        }
	}
    if (second_stage && --second_stage == 0){
        exit_flag = 1;
    }

}


int init(int moduleno, char* argstr) {
	int mx = matrix_getx();
	int my = matrix_gety();
	data = malloc(sizeof(int) * mx * my);
	data_bitmask = malloc((mx*my+1)/8);
	modno = moduleno;
	frame = 0;
    timer_n = 0;
	return 0;
}

static void own_reset(){
	fill_data();
    generate_settings();

    // timing
#ifdef SORT_TIMING
    if (timer_n)
        printf("Avg: sort %luus, draw %luus",td1_acc/timer_n,td2_acc/timer_n);
#endif
    td1_acc=0;
    td2_acc=0;
    timer_n=0;
}


void reset(int _modno) {
	own_reset();
	nexttick = udate();
	matrix_clear();
}

int draw(int _modno, int argc, char* argv[]) {
	int mx = matrix_getx();
	int my = matrix_gety();

#ifdef SORT_TIMING
    t1 = udate();
	sort_data();
    t2 = udate();
#else
    sort_data();
#endif

	for (int i=0;i<mx;i++){
		for (int j=0;j<my;j++){
#ifdef USE_BITMASK
            int doffset = i+mx*j;
            if (data_bitmask[doffset/8] & (1<<(doffset%8)))
#endif
                matrix_set(i,j,colorwheel(data[i+mx*j]));
            //    matrix_set(i,j,RGB(255,255,255));
            //else
            //    matrix_set(i,j,RGB(0,0,0));
		}
	}
#ifdef USE_BITMASK
    for (int i = 0;i<(mx*my)/8;i++) data_bitmask[i] = 0;
#endif

#ifdef SORT_TIMING
    t3 = udate();
    td1_acc += (t2-t1);
    td2_acc += (t3-t2);
    timer_n += 1;
#endif


	if (exit_flag){
		own_reset();
		return 1;
	}

	matrix_render();

	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void deinit(int _modno) {
	free(data);
    free(data_bitmask);
}
