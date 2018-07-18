// Sorting visualizations

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#define FPS 300
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

static int modno;
static ulong frame;
static ulong nexttick;
static int mx, my;

static int * data;

static int sorting_algorithm=5;

// highlighting
static int h1;
static int h2;


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
    case 0: return RGB(255,v,0);
    case 1: return RGB(255-v,255,0);
    case 2: return RGB(0,255,v);
    case 3: return RGB(0,255-v,255);
    case 4: return RGB(v,0,255);
    case 5: return RGB(255,0,255-v);
    }
}

// sorting algorithm internals
static int i,j;
static int inversions;
static int gap;
static int iMin;
static int start,end,child,root,swapable;
static int last;

static int coctail_shaker_sort(){
    CONTINUE(1);
    CONTINUE(2);
    start = 0;
    end = mx-1;
    while(1) {
        inversions = 0;
        for (i=start;i<end;i++) {
            if (data[i] < data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(1);
        }
        end = last;
        if (!inversions) break;
        inversions = 0;
        for (i=end-1;i>=start;i--){
            if (data[i] < data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(2);
        }
        start = last;
        if (!inversions) break;
    }
    return 1;
}

static int heapsort(){
    CONTINUE(1);
    CONTINUE(2);
    //CONTINUE(3);
    CONTINUE(4);
    CONTINUE(5);

    // heapify
    for (start=mx/2; start>=0;start--) {
        end = mx-1;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(1);
            if (data[swapable] > data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(2);
                if (data[swapable] > data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    // heapsort
    for (end=mx-1;end > 0;) {
        swap(end,0);
        //h1=0;h2=end;YIELD(3);
        end--;
        start = 0;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(4);
            if (data[swapable] > data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(5);
                if (data[swapable] > data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    return 1;
}


static int bubblesort() {
    CONTINUE(1);
    for (j = mx-1; j>0; j--) {
        inversions = 0;
        for (i = 0; i<j; i++) {
            cmp_swap(i,i+1);
            YIELD(1);
        }
        if (inversions == 0) {
            return 1;
        }
    }
    return 1;
}

static int selection_sort() {
    CONTINUE(1);
    CONTINUE(2);
    for (j = 0; j < mx-1; j++) {
        iMin = j;
        h1 = iMin;
        for (i = j+1; i < mx; i++) {
            h2 = i;
            YIELD(1);
            if (data[i] > data[iMin]) {
                iMin = i;
                h1 = iMin;
            }
        }
        h2 = j;
        swap(j, iMin);
        YIELD(2);
    }
    return 1;
}

static int insertion_sort() {
    CONTINUE(1);
    for (i=1; i<mx; i++) {
        for (j=i; j>0&&data[j-1]<data[j]; j--) {
            cmp_swap(j-1,j);
            YIELD(1);
        }
    }
    return 1;
}

static int comb_sort() {
    CONTINUE(1);
    gap = mx;
    do {
        gap = (int)floor(gap/1.3);
        for (i=0; i+gap<mx; i++) {
            cmp_swap(i,i+gap);
            YIELD(1);
        }
    } while (gap > 0);
    return 1;
}




static int sort() {
    switch (sorting_algorithm) {
    case 0: return bubblesort();
    case 1: return comb_sort();
    case 2: return insertion_sort();
    case 3: return selection_sort();
    case 4: return heapsort();
    case 5: return coctail_shaker_sort();
    default: return bubblesort();
    }
}


int draw(int argc, char* argv[]) {
    matrix_clear();
    int rval = 0;
    rval = sort();
    if (h1 >= 0 || h2 >= 0) {
        for (int y=0; y<my; y++) {
            if (h1 >= 0) matrix_set(h1,y,RGB(80,80,80));
            if (h2 >= 0) matrix_set(h2,y,RGB(80,80,80));
        }
    }

    for (int x=0; x<mx; x++) {
        int range = data[x]-2;
        for (int y=my-1; y>range; y--) {
            RGB color = colorwheel(data[x]*1000/mx);
            matrix_set(x,y,color);
        }
    }

    matrix_render();

    if (rval > 0) return 1;
    frame++;
    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void reset(void) {
    data[0] = 1;
    for (int i=1; i<mx; i++) {
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
    sorting_algorithm = randn(5);
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
