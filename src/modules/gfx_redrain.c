// Please modify me as you please
// Additional variations are welcome
// have fun!

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <graphics.h>
#include <math.h>

#define FPS 100
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

#define REDNESS_RANGE 100
#define REDNESS_OFFSET 50
#define NO_OF_PARTICLES mx/4

static int modno;
static ulong frame;
static ulong nexttick;

static int mx,my;
static int bx,by;
static int redness;

typedef struct drop {
    int x;
    int y;
    int redness;
} drop;

static drop * drops;

static void pdraw(int x, int y, int r, int g, int b){
        if (x<0) x+=mx;
        if (x>=mx) x-=mx;
        if (y<0) y+=my;
        if (y>=my) y-=my;
        matrix_set(x,y,RGB(r,g,b));
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    modno = moduleno;
    frame = 0;
    drops = malloc(NO_OF_PARTICLES * sizeof(drop));
    return 0;
}


void reset(void) {
    nexttick = udate();
    //matrix_clear();
    for (int i = 0;i<NO_OF_PARTICLES;i++){
        drops[i].x = randn(mx-1);
        drops[i].y = randn(my-1);
        drops[i].redness = randn(REDNESS_RANGE);
    }
    frame = 0;
}

static void randwalkdraw(){
    for (int i=0;i<NO_OF_PARTICLES;i++){
        //if (randn(2)) continue;
        drops[i].y+=1;
        drops[i].x+=randn(2)-1;
        if (drops[i].x<0) drops[i].x+=mx;
        if (drops[i].x>=mx) drops[i].x-=mx;
        if (drops[i].y<0) drops[i].y+=my;
        if (drops[i].y>=my) drops[i].y-=my;
        drops[i].redness+=randn(10)-5;
        if (drops[i].redness < 0) drops[i].redness =0;
        if (drops[i].redness > REDNESS_RANGE) drops[i].redness =REDNESS_RANGE;
        int red = REDNESS_OFFSET + drops[i].redness;
        pdraw(drops[i].x,drops[i].y,red,0,0);
#define FAT_DROPS
#ifdef FAT_DROPS
        pdraw(drops[i].x,drops[i].y+1,red*1.2,0,0);
        pdraw(drops[i].x-1,drops[i].y,red*0.8,0,0);
        pdraw(drops[i].x+1,drops[i].y,red*0.8,0,0);
        pdraw(drops[i].x-1,drops[i].y+1,red*0.5,0,0);
        pdraw(drops[i].x+1,drops[i].y+1,red*0.5,0,0);
#endif
    }
}

int draw(int argc, char* argv[]) {

    randwalkdraw();

    

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
    free(drops);
    return 0;
}
