// a kind of 2D Bubblesort
// The sorting Network for 2x2 px looks like:
// A B
// |X   
// C D
//
// This is applied to random points

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
#define FRAMES (RANDOM_TIME * FPS) * 10

// calculate and print timing information
#define SORT_TIMING
// Use bitmask to store where updates happened
// Seems faster without on an i5
//#define USE_BITMASK
static int modno;
static ulong frame;
static ulong nexttick;

static ulong t1;
static ulong t2;
static ulong t3;
static ulong td1_acc;
static ulong td2_acc;
static ulong timer_n;

static int * data;
static char * data_bitmask;
static int dir;
static int second_stage;
static int comparisons_hot;
static int comparisons_cold;
static int exit_flag;

// SETTINGS
static const int color_range = 700;
static const int generator_step = 8;
static const int boring_percentage = 8;


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
	}
}

static RGB randcolor(){
	return colorwheel(randn(1536));
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
		case 1:
			scmp(d,a);
			scmp(c,b);
			scmp(d,c);
			scmp(b,a);
			//scmp(d,a);
			//scmp(c,b);
			break;
		default:
			scmp(a,d);
			scmp(c,b);
			scmp(a,c);
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
            int step = entropy % generator_step;
            entropy /= generator_step;
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
        if (dir == 0){
            dir = 1;
            second_stage = frame+frame/3;
        } else {
            exit_flag = 1;
        }
	}
    if (dir == 1 && --second_stage <= 0) {
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
	frame = 0;
	dir = 0;
	second_stage=0;
    exit_flag=0;

    // timing
#ifdef SORT_TIMING
    if (timer_n)
        printf("Avg: sort %dus, draw %dus",td1_acc/timer_n,td2_acc/timer_n);
#endif
    td1_acc=0;
    td2_acc=0;
    timer_n=0;
}


void reset(void) {
	own_reset();
	nexttick = udate();
	matrix_clear();
}

int draw(int argc, char* argv[]) {
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
            int doffset = i+mx*j;
#ifdef USE_BITMASK
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

int deinit() {
	free(data);
    free(data_bitmask);
	return 0;
}
