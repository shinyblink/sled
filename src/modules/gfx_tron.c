// Self running Tron Game
// Copyright (c) 2020, Jonathan Cyriax Brast
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

#define FPS 10
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

static int modno;
static int frame;
static oscore_time nexttick;


static int players;
static char* field;
static int fx;
static int fy;
static int mx;
static int my;
static int scale;
static RGB* colors1;
static RGB* colors2;
static int * positions;

static RGB black = RGB(0, 0, 0);

static RGB colorwheel(int angle){
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
    default: return RGB(0,0,0);
    }
}

int init(int moduleno, char* argstr) {
	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

    players = 8;
    // WARNING: will not terminate for 1536 / players < 200
    scale = 4;
    fx = mx/scale;
    fy = my/scale;
    field = malloc(sizeof(char)*fx*fy);
    colors1 = malloc(sizeof(RGB)*players);
    colors2 = malloc(sizeof(RGB)*players);
    positions = malloc(sizeof(int)*players);

	modno = moduleno;
	frame = 0;
	return 0;
}




#define max(a,b) ((a<b)?b:a)
#define min(a,b) ((a>b)?b:a)
static int player_ai(int p){
    int pos0 = positions[p];
    if (pos0 < 0) return;
    int v[4] = {100,100,100,100};

    // make a random direction preferences
    int i1 = randn(3);
    int i2 = randn(2);
    int i3 = randn(1);
    if (i1 <= i2) i2 += 1;
    if (i1 <= i3 || i2 <= i3) i3 += 1;
    if (i1 == i3 || i2 == i3) i3 += 1;
    v[i3] += 3; v[i2] += 2; v[i1] += 1;

    // check walls
    int x,y;
    x = pos0%fx;
    y = pos0/fx;
    if (x == 0)     v[1] = -10000;
    if (x == fx-1)  v[2] = -10000;
    if (y == 0)     v[0] = -10000;
    if (y == fy-1)  v[3] = -10000;

    // check players
    if (v[0] > 0 && field[pos0-fx] != 0) v[0] = -11000;
    if (v[1] > 0 && field[pos0- 1] != 0) v[1] = -11000;
    if (v[2] > 0 && field[pos0+ 1] != 0) v[2] = -11000;
    if (v[3] > 0 && field[pos0+fx] != 0) v[3] = -11000;






    // choose best
    int best = max(max(v[0],v[1]),max(v[2],v[3]));
    //printf("p%d @(%d,%d): %d %d %d %d (%d)\n",p,x,y,v[0],v[1],v[2],v[3], best);
    if (v[0] == best) return 0;
    if (v[1] == best) return 1;
    if (v[2] == best) return 2;
    if (v[3] == best) return 3;
    return 0;
}

static void draw_pos(int pos, RGB color){
    int x,y;
    x = (pos%fx)*scale;
    y = (pos/fx)*scale;
    for (int ox = 0; ox < scale;ox++){
        for (int oy = 0; oy < scale; oy++){
            matrix_set(x+ox,y+oy,color);
        }
    }
}
static void draw_highlight(int pos0, int pos1, RGB color){
    int x1,y1,x2,y2,xA,xB,yA,yB;
    x1 = (pos0%fx)*scale;
    y1 = (pos0/fx)*scale;
    x2 = (pos1%fx)*scale;
    y2 = (pos1/fx)*scale;
    xA = min(x1,x2)+1;
    xB = max(x1,x2)+3;
    yA = min(y1,y2)+1;
    yB = max(y1,y2)+3;
    for (int x=xA;x<xB;x++){
        for (int y=yA;y<yB;y++){
            matrix_set(x,y,color);
        }
    }
}

static void remove_player(int p){
    for (int i = 0; i < fx*fy; i++){
        if (field[i] == p+1){
            draw_pos(i, black);
            field[i] == 0;
        }
    }

}


static void move_and_draw_player(int p, int dir){
    if (positions[p] < 0) return;
    int pos0 = positions[p];
    int directions[4] = {-fx,-1,+1,+fx};
    int pos1 = directions[dir]+pos0;
    //printf("p%d %d->%d (d:%d)\n",p,pos0,pos1, dir);
    if (    pos1 < 0 // -y out of bounds
            || pos1 >= fx*fy // +y out of bounds
            || (pos0%fx == 0 && dir == 1) // -x out out bounds
            || ((pos0%fx)== fx-1 && dir == 2) // +x out of bounds
            || field[pos1] != 0 // collision
       ){
        //printf("p%d DEAD\n",p);
        remove_player(p);
        positions[p] = -1;
        return;
    }
    // just draw new position for now
    draw_pos(pos1,colors2[p]);
    draw_highlight(pos0,pos1,colors1[p]);
    positions[p] = pos1;
    field[pos1] = p+1;
}


int draw(int _modno, int argc, char* argv[]) {

    for (int p = 0; p < players; p++){
        move_and_draw_player(p,player_ai(p));
    }

	matrix_render();

    int alive = 0;
    for (int p = 0; p < players; p++){
        if (positions[p] > 0 ) alive += 1;
    }

	if (alive < 2) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

void reset(int _modno) {
	matrix_clear();
    for (int i = 0;i < fx*fy; i++){
        field[i] = 0;
    }
    int pcolor[players];
    for (int p = 0; p < players;p++){
        int collision_flag = 1; 
        while (collision_flag) {
            positions[p] = randn(fx*fy-1);
            collision_flag = 0;
            for (int p2 = 0; p2 < p; p2++){
                if (positions[p] == positions[p2]) collision_flag = 1;
            }
        }
        field[positions[p]] = p+1;
        collision_flag = 1;
        while (collision_flag) {
            pcolor[p] = randn(1535);
            collision_flag = 0;
            for (int p2 = 0; p2 < p; p2++){
                int cdiff = pcolor[p] - pcolor[p2];
                if (cdiff < 0) cdiff = 0 - cdiff;
                if (cdiff < 100) collision_flag = 1;
            }
        }
        RGB c = colorwheel(pcolor[p]);
        colors1[p] = c;
        colors2[p] = RGB(c.red/2,c.green/2,c.blue/2);
        draw_pos(positions[p],colors2[p]);
    }

	nexttick = udate();
	frame = 0;
}

void deinit(int _modno) {
	free(field);
    free(colors1);
    free(colors2);
    free(positions);
}
