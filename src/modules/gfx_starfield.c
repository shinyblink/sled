// Starfield Simulation 1
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
#include <stdio.h>
#include <math.h>

#define FPS 30
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

static int modno;
static int frame;
static oscore_time nexttick;

static int mx;
static int my;
static int scale;

#define INVERSE_STAR_DENSITY 100;

#define MOVEMENT_6DOF

static const float z_speed = 0.05;
#ifdef MOVEMENT_6DOF
static float x_speed = 0.00;
static float y_speed = 0.0;
static float roll_speed = 0.01;
static float pitch_speed = 0.0;
static float yaw_speed = 0.0;
static float target_roll_speed = 0.01;
static float target_pitch_speed = 0.0;
static float target_yaw_speed = 0.0;
static float cos_roll;
static float sin_roll;
static float cos_pitch;
static float sin_pitch;
static float cos_yaw;
static float sin_yaw;
#endif


typedef struct star_t {
    float fx;
    float fy;
    float fz;
    int brightness;
} star_t;


static int no_of_stars ;
static star_t * stars = 0;

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    scale = (mx<my)?mx:my;
    no_of_stars = (mx*my)/INVERSE_STAR_DENSITY;
    stars = malloc(no_of_stars*sizeof(star_t));
    modno = moduleno;
    frame = 0;
    return 0;
}

void randomize_star(star_t * star) {
    uint rr = rand();
    int r;
    r = rr & 0xff;
    rr >>= 8;
    star->fz = 0.3+r*1.0/64.;

    r = rr & 0xff;
    rr >>= 8;
    r -= 128;
    star->fx = r*star->fz/128.0*0.5;

    r = rr & 0xff;
    rr >>= 8;
    r -= 128;
    star->fy = r*star->fz/128.0*0.5;

    r = rr & 0xff;
    rr >>= 8;
    star->brightness = 128 + r/2;
}

void rerandomize_star(star_t * star) {

}

void reset(int _modno) {
    for (star_t * star=stars; star < stars + no_of_stars; star++) {
        randomize_star(star);
    }
    nexttick = udate();
    matrix_clear();
    frame = 0;
}

void apply_transformation(star_t * star) {
    star->fz -= z_speed;
#ifdef MOVEMENT_6DOF
    star->fx -= x_speed;
    star->fy -= y_speed;
    if (roll_speed != 0.0) {
        float x = star->fx;
        float y = star->fy;
        star->fx =  cos_roll*x + sin_roll*y;
        star->fy = -sin_roll*x + cos_roll*y;
    }
    if (pitch_speed != 0.0) {
        float y = star->fy;
        float z = star->fz;
        star->fy =  cos_pitch*y + sin_pitch*z;
        star->fz = -sin_pitch*y + cos_pitch*z;
    }
    if (yaw_speed != 0.0) {
        float x = star->fx;
        float z = star->fz;
        star->fx =  cos_yaw*x + sin_yaw*z;
        star->fz = -sin_yaw*x + cos_yaw*z;
    }
#endif

}

#ifdef MOVEMENT_6DOF
static void prepare_transformation() {
    cos_roll = cos(roll_speed);
    sin_roll = sin(roll_speed);
    cos_pitch = cos(pitch_speed);
    sin_pitch = sin(pitch_speed);
    cos_yaw = cos(yaw_speed);
    sin_yaw = sin(yaw_speed);
}
#else
static void prepare_transformation() {}
#endif

int draw(int _modno, int argc, char* argv[]) {
    matrix_clear();

    for (star_t * star=stars; star < stars + no_of_stars; star++) {
        int px = (int)(star->fx * scale / star->fz)+mx/2;
        int py = (int)(star->fy * scale / star->fz)+my/2;
        prepare_transformation();
        apply_transformation(star);
        //printf("%+2f %+2f %+2f\n",star->fx,star->fy,star->fz);
        if (px < 0 || px >= mx || py < 0 || py >= my||star->fz < 0) {
            randomize_star(star);
            continue;
        }
        int brightness = (star->brightness/star->fz)+64;
        matrix_set(px,py,RGB(brightness,brightness,brightness));
    }
    roll_speed = 0.9 * roll_speed + 0.1 * target_roll_speed;
    pitch_speed = 0.9 * pitch_speed + 0.1 * target_pitch_speed;
    yaw_speed = 0.9 * yaw_speed + 0.1 * target_yaw_speed;


    matrix_render();

#ifdef MOVEMENT_6DOF
    int r=rand();
    if (0 == (r & 0x1f)) {
        if (r&1)
            target_roll_speed = 0.0;
        else
            target_roll_speed = 0.002 * (((r>>15)&0xf)-8);
        //printf("roll: %f\n",roll_speed);
    }
    if (0 == (r & 0x3e0)) {
        if (r&1)
            target_pitch_speed = 0.0;
        else
            target_pitch_speed = 0.002 * (((r>>15)&0xf)-8);
        //printf("pitch: %f\n",pitch_speed);
    }
    if (0 == (r & 0x7c00)) {
        if (r&1)
            target_yaw_speed = 0.0;
        else
            target_yaw_speed = 0.002 * (((r>>15)&0xf)-8);
        //printf("yaw: %f\n",yaw_speed);

    }
#endif


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
    free(stars);
}
