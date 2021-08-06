// Please modify me as you please
// Additional variations are welcome
// have fun!
//
// Copyright (c) 2021, Jonathan Cyriax Brast
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
#include <graphics.h>
#include <math.h>

#define FPS 15
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx,my;

static int p = 4;
static int p_max = 8;
static float * intensities;
static float * intensities2;

static RGB intense_red(int intensity){
    if (intensity > 511) intensity = 511;
    if (intensity < 256) {
        return RGB(intensity/2,0,intensity/2);
    } else {
        return RGB(255,intensity-256,intensity/2);
    }
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    intensities = malloc(sizeof(int)*mx*my);
    intensities2 = malloc(sizeof(int)*mx*my);
    for (float * i =intensities;i<intensities+(mx*my);i++){
        *i = + randn(100)/1000.0;
    }
    modno = moduleno;
    frame = 0;
    return 0;
}


void reset(int _modno) {
    nexttick = udate();
    p = (p+1+randn(p_max))%(p_max+1);
    //printf("%d",p);
    if (p >= 5){
        for (float * i =intensities;i<intensities+(mx*my);i++){
            *i = + randn(1000)/1000.0;
        }
    }
    frame = 0;
}

void update_cell(int x, int y){
    float a = 0, b = 0, c = 0, d = 0;
    float am = 0, bm = 0, cm = 0, dm = 0;
    int aa=0,bb=0,cc=0,dd=0;
    float val = 0;
    for (int dx = -8;dx <= 8; dx++){
        for (int dy = -8;dy <= 8; dy++){
            int dist = dx*dx+dy*dy;
            int ox = x+dx, oy = y+dy;
            if (ox<0) ox += mx;
            if (ox>=mx) ox -= mx;
            if (oy<0) oy += my;
            if (oy>=my) oy -= my;
            float other_cell = intensities[ox+oy*mx];

            if (dist == 0) {
                a += other_cell;
                am = am > a ? am : a; 
                aa++;
            } else if (dist < 20) {
                b += other_cell;
                bm = bm > b ? bm : b; 
                bb++;
            } else if (dist < 60) {
                c += other_cell;
                cm = cm > c ? cm : c; 
                cc++;
            } else if (dist < 128){
                d += other_cell;
                dm = dm > d ? dm : d; 
                dd++;
            }
        }
    }
    a /= aa;
    b /= bb;
    c /= cc;
    d /= dd;
    //p=8;
    if (p==0){
        val = a*0.8+0.2;
        if (b > 4) val *= 0.02;
        val += b>c? b-c:c-b;
        val -= c*0.3;
    }
    if (p==1){
        val = a*0.8+0.2;
        if (b > 2) val /= (b-2.01)*100;
        val += b>c? b-c*1:c-b;
        val -= c*0.3;
    }
    if (p==2){
        val = a*0.8+0.2;
        if (b > 2) val /= (b-2.01)*200;
        val += b>c? b-c*0.4:c-b;
        val -= c*0.3;
    }
    if (p==3){
        val = a*0.8+0.2;
        if (b > 2) val /= (b-2.01)*100;
        val += b>c? b-c*1.0:c-b*4;
        val -= c*0.3;
    }
    if (p==4){
        val = a*0.44+0.2;
        if (c > 0.5 && c < 0.8) val += a*(0.5-c)*(c-0.8)*(30.85-c+1);
        if (a > b ) val -= 0.01;
        if (val < .36) val = .5+randn(1000)/1000.;
    }
    if (p==5){
        val = a*0.80 + 0.2*(b*0.95 + c *0.05) + 0.15 + 0.001*randn(10);
        if (val > 2) val -= 1.8 + 0.01*val;
        //if (c < 1.5) val += 0.05;
        //if (b < 1) val += 0.05;
    }
    if (p==6){
        val = a;
        if (b > .185 && b < .2) val += 0.1;
        if (b > .343 && b < .58) val -= 0.1;
        if (b > .75 && b < .85) val -= 0.1;
        if (c > .15 && c < .28) val -= 0.1;
        if (c > .445 && c < .68) val += 0.1;
        if (b > .75 && b < .18) val -= 0.1;
        if (a > 1) val -= 0.01*a;
        if (a > 2) val -= 0.02*(a-1);
        if (a > 3) val -= 0.03*(a-2);
    }
    if (p==7){
        val = a;
        if (a > 1) val -= 0.01*a;
        if (a > 2) val += 0.02*(a-1);
        if (a > 3) val -= 0.04*(a-2);
        if (a > 3.5) val = 0.5;
        if (.18 < b && b < .2) val += 0.1;
        if (.46 < b && b < .5) val -= 0.1;
        if (.52 < b && b < .6) val += 0.1;
    }
    if (p==8){
        val = a;
        if (a > bm) val = bm+0.1*(a-bm);
        if (a > 1) val -= 0.01*a;
        if (a > 2) val -= 0.02*(a-1);
        if (a > 3) val -= 0.04*(a-2);
        if (a > 3.5) val = 0.5;
        if (.18 < b && b < .2) val += 0.1;
        if (.46 < b && b < .51) val -= 0.1;
        if (.46 < c && c < .7) val -= 0.1;
        if (.93 < c && c < 1.2) val += 0.1;
        if (1.3 < c && c < 1.8) val -= 0.1;
        if (.48 < b && b < .6) val += 0.1;
        if (.50 < d && d < .70) val += 0.1; 
        if (0.5 > bm) val += 0.1;
        if (2.4 < cm) val -= 0.1/cc;
        //if (2.4 > dm) val -= 0.1/cc;
    }
    if (val < 0) val = 0;
    //val = 5*exp(val)/(exp(val)+1);
    //if (val > 4) val = 5*val/(1+val);
    //printf("%f %f %f %f %f\n",a,b,c,d,val); 
    intensities2[x+mx*y] = val;
}

int draw(int _modno, int argc, char* argv[]) {
    for (int x = 0; x<mx; x++) {
        for (int y=0; y<my; y++) {
            update_cell(x,y);
        }
    }
    for (int i = 0; i < mx*my; i++){
        intensities[i] = intensities2[i];
    }
    for (int x = 0; x<mx; x++) {
        for (int y=0; y<my; y++) {
            RGB res = intense_red(80*intensities[x+y*mx]);
            matrix_set(x,y,res);
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
    free(intensities);
    free(intensities2);
}
