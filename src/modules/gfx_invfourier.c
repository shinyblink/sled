// Simple inverse FFT
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
#include <math.h>

#define FPS 20
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static int frame;
static int mx, my;
static float sc;
static ulong nexttick;

typedef struct speclet {
	float pos_ph;
	float pos_rd;
    float v_ph;
    float v_rd;
	float phase;
	float radius;
    float v_radius;
} speclet;
static float voffset, xoffset, yoffset,dxoffset,dyoffset;

static int numspeclets;
static speclet* spectrum;

static float randf() {
    return (1.0*randn(200000)-100000.0)/100000.0;
}


static RGB dim(RGB c,float val){
    return RGB(c.red*val,c.green*val,c.blue*val);
}
static RGB colorwheel(float fangle){
    if (fangle < 0) fangle += roundf(-fangle);
    if (fangle < 0) fangle += 1;
    int angle = (int)(fmodf(fangle,1.0)*1536);
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

	numspeclets = 10;
	spectrum = malloc(numspeclets * sizeof(speclet));

	modno = moduleno;
	frame = 0;
	return 0;
}

static void randomize(void) {
    sc = randf()*6;
    for (speclet * s=spectrum;s<spectrum+numspeclets;s++){
        s->pos_ph = randf();
        s->pos_rd = randf()*6.3;
        s->v_ph = randf()/10.0;
        s->v_rd = randf()/10.0;
        s->phase  = randf()*6.3;
        s->radius = randf();
        s->v_radius = randf()/10.0;
	}
    xoffset=0;
    yoffset=0;
    dxoffset=randf()/10.0;
    dyoffset=randf()/10.0;
    voffset = randf();
}

static void update(void) {
    for (speclet * s=spectrum;s<spectrum+numspeclets;s++){
        s->pos_ph += s->v_ph;
        s->pos_rd += s->v_rd;
        s->pos_rd = 0.01 + 0.99*s->pos_rd;
        s->v_ph += randf()/40.0;
        s->v_rd += randf()/40.0;
        s->v_ph *= 0.9;
        s->v_rd *= 0.9;
        s->radius += s->v_radius;
        s->v_radius += randf()/10.0;
        s->v_radius *= 0.9;
        s->radius = (0.01+0.99*s->radius);
        s->radius /= 1.0+(s->pos_rd*s->pos_rd+s->pos_rd+0.25)*0.01;
        s->phase += randf()/10.0;
    }
    xoffset += dxoffset;
    yoffset += dyoffset;
    xoffset *= 0.95;
    yoffset *= 0.95;
    dxoffset += randf()/10.0;
    dyoffset += randf()/10.0;
    voffset += 0.01;
}

void reset(int _modno) {
	nexttick = udate();
	matrix_clear();
	randomize();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
    update();
    for (int x =0;x<mx;x++){
        for (int y=0;y<my;y++){
            float px=(x-mx/2.0)-xoffset,py=(y-my/2.0)-yoffset;
            float val = 0;
            for (speclet * s=spectrum;s<spectrum+numspeclets;s++){
                float xx=px,yy=py;
                float vx=s->pos_rd*sinf(s->pos_ph);
                float vy=s->pos_rd*cosf(s->pos_ph);
                val += s->radius*sinf((vx*xx)/(sc*sc)+(vy*yy)/(sc*sc)+s->phase);

            }
            matrix_set(x, y, dim(colorwheel(0.3*val/numspeclets+voffset),0.5));
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

void deinit(int _modno) {
	free(spectrum);
}
