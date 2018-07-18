// Sorting visualizations

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define FPS 100
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

static int modno;
static ulong frame;
static ulong nexttick;
static int mx, my;

static int * data;

static int sorting_algorithm=1;

// highlighting
static int h1;
static int h2;

// sorting algorithm internals
static int i,j;
static int inversions;
static int gap;

// pseudo jump and link
static int __yield_value;
#define CONTINUE(x)         \
    if (__yield_value == x) \
        goto label##x;
#define YIELD(x)        \
    __yield_value = x;  \
    return 0;           \
    label##x:

#define swap(a,b)       \
    int __tmp = data[a];\
    data[a] = data[b];  \
    data[b] = __tmp;

#define cmp_swap(a,b)       \
    assert(a<mx); assert(b<mx);\
    h1 = a; h2 = b;         \
    if (data[a] < data[b]){ \
        swap(a,b);          \
        inversions++;       \
    }


RGB colorwheel(int angle) {
    angle = angle % 1536;
    int t = (angle / 256)%6;
    int v = angle % 256;
    switch (t) {
    case 0:
        return RGB(255,v,0);
    case 1:
        return RGB(255-v,255,0);
    case 2:
        return RGB(0,255,v);
    case 3:
        return RGB(0,255-v,255);
    case 4:
        return RGB(v,0,255);
    case 5:
        return RGB(255,0,255-v);
    }
}

static int bubblesort(){
    CONTINUE(1);
    h1 = -1;
    h2 = -1;
    for (j = mx-1;j>0;j--){
        inversions = 0;
        for (i = 0;i<j;i++){
            cmp_swap(i,i+1);
            YIELD(1);
        }
        if (inversions == 0){
            return 1;
        }
    }
    return 1;
}

static int insertion_sort(){
    CONTINUE(1);
    gap = mx;
    do {
        inversions = 0;
        gap = (int)floor(gap/1.3);
        for (i=0;i+gap<mx;i++){
            cmp_swap(i,i+gap);
            YIELD(1);
        }
    } while (gap > 1 && inversions > 0);
    return 1;
}

static int sort(){
    switch (sorting_algorithm){
        case 0: return bubblesort();
        case 1: return insertion_sort();
        default: return bubblesort();
    }
}


int draw(int argc, char* argv[]) {
    matrix_clear();
    int rval = 0;
    rval = sort();
    if (h1 >= 0 || h2 >= 0){
        for (int y=0;y<my;y++){
            if (h1 >= 0) matrix_set(h1,y,RGB(80,80,80));
            if (h2 >= 0) matrix_set(h2,y,RGB(80,80,80));
        }
    }

    for (int i=0; i<mx; i++) {
        int range = data[i]-2;
        for (int j=my-1; j>range; j--) {
            RGB color = colorwheel(data[i]*1000/mx);
            matrix_set(i,j,color);
        }
    }

    matrix_render();


    frame++;
    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return rval;
}

void reset(void) {
    data[0] = 1;
    for (int i=1;i<mx;i++){
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
    nexttick = udate();
    matrix_clear();
    frame = 0;
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    data = malloc(mx * sizeof(int));
    modno = moduleno;
    frame = 0;
    return 0;
}

int deinit() {
    free(data);
    return 0;
}
