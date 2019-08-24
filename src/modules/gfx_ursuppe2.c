// Derived from bttrblls
// Draws particles with peculiar behavior
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
#include <stdio.h>
#include <math.h>

static int die = 0.0;
#define SHOT printf("%i\n",__LINE__); // for shotgun debugging
#define SHOTNAN(varname) if(isnan(varname)) printf("%i: %s is nan\n",__LINE__,#varname);\
    //fflush(0);printf("%d",1/die);


#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

#define _min(a,b) a<b?a:b
#define cmp_swap(a,b)\
    if (a > b){__tmp = a;a = b;b = __tmp;}
#define _sort4(_type,x1,x2,x3,x4) \
    {_type __tmp;\
    cmp_swap(x1,x2);cmp_swap(x3,x4);\
    cmp_swap(x1,x3);cmp_swap(x2,x4);\
    cmp_swap(x2,x3);}

#define TYPES(t) particle_type * t = types; t < types + numtypes; t++
#define PARTICLES(p) particle * p = particles; p < particles + numparticles; p++

static int modno;
static int frame;
static oscore_time nexttick;

typedef struct force_curve {
    float range[4];
    float force[4];
} force_curve;

typedef struct particle_type {
    int rgb_offset;
    force_curve * forces;
} particle_type;

typedef struct particle {
	RGB color;
	float pos_x;
	float pos_y;
	float vel_x;
	float vel_y;
    int type;
} particle;

static int numtypes;
static int numparticles;
static particle_type * types;
static particle* particles;

static int maxrange = 32;

static int mx;
static int my;

typedef char bool;
#define true 1
#define false 0
static bool USE_ADDITIVE_COLOR = false;
static bool USE_POTENTIAL = true;
static bool USE_PULSATION = false;
static bool USE_DAMPENING = false;
static bool USE_STOKES_FRICTION = true;
static bool USE_ROTATION = false;
static bool USE_PULSATION_DAMPENING = false;
static bool USE_POTENTIAL_DAMPENING = false;
static bool USE_TRAILS = true;
static bool USE_WIGGLE = true;
static bool USE_COLOR = true;
static int TRAIL_LENGTH = 230;
static float DAMPENING_CONSTANT = 0.98;
static float POTENTIAL_SIZE = 0.9;
static float POTENTIAL_ECCENTRICITY = 0.8;
static float FORCE_BIAS = -0.8;
static float FERMIOTIC_FORCE = 100;
static float FERMIOTIC_RANGE = 0.5;
static float PP_FORCE_MULTIPLIER = 0.1;


static void mset(float x, float y, RGB color){
    int x_i = (int) x;
    int y_i = (int) y;
    if (x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety())
        matrix_set(x_i,y_i, color);
}

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
    }
    return RGB(0,0,0); // Should not happen
}

int init(int moduleno, char* argstr) {
    die = 10;
    while (--die){}

	mx = matrix_getx();
	my = matrix_gety();

	if ((mx * my) < 16)
		return 1;

    numparticles = (mx+my) /10;
	particles = malloc(numparticles * sizeof(particle));

    numtypes = 4;
    types = (particle_type*) malloc(numtypes * sizeof(particle_type));

    for (int i = 0; i < numtypes;i++){
        types[i].forces = malloc(numtypes * sizeof(force_curve));
    }

	modno = moduleno;
	frame = 0;
	return 0;
}

static void randomize(void) {
    for (TYPES(t)){
        for (force_curve * f = t->forces; f<t->forces+numtypes; f++){
            for (int i = 0; i < 4;i++){
                f->range[i] = (float)randn(maxrange*8)/8.0;
                f->force[i] = (float)randn(32)/16.0-1.0-FORCE_BIAS;
            }
            _sort4(float,f->range[0],f->range[1],f->range[2],f->range[3]);
            //_sort4(float,f->force[0],f->force[1],f->force[2],f->force[3]);
            f->range[0] /= 8.0;
            f->range[1] /= 4.0;
            f->range[2] /= 2.0;
        }
    }
    printf("matrix: %d %d\n",mx,my);
    for (PARTICLES(p)){
        p->type = randn(numtypes-1);
        p->color = colorwheel(types[p->type].rgb_offset + randn(512));
        int x = randn(mx-1);
        int y = randn(my-1);
        int rx = +(y - my/2);
        int ry = -(x - mx/2);
		p->pos_x = (float)x;
        p->pos_y = (float)y;
        p->vel_x = ((float)randn(8))/10.0 * rx/mx;
        p->vel_y = ((float)randn(8))/10.0 * ry/my;

	}
}

static float get_force(const force_curve const * fc, const float distance){
    if (distance > maxrange) return 0.0;
    if (distance < FERMIOTIC_RANGE) return FERMIOTIC_FORCE;
    float r1,r2,f1,f2;
    r1 = 0;
    f1 = FERMIOTIC_FORCE;
    for (int i=0;i<5;i++){
        if (i>=4){
            r2 = (float)maxrange;
            f2 = 0;
            break;
        }
        r2 = fc->range[i];
        f2 = fc->force[i];
        if (r1 <= distance && r2 >= distance) break;
        r1 = r2; f1 = f2;
    }
    if (r1 == r2) {
        return 0.0;
    }
    float force = (distance - r1)/(r2-r1)*(f2-f1);
    return force;
}

static void update(void) {
    float slow_phase = cosf(frame/100.0);
    slow_phase *= slow_phase;
    float fast_phase = cosf(frame/2.0);

    // Particle-Particle Forces
    for (PARTICLES(p1)){
        for (PARTICLES(p2)){
            if (p1 == p2) continue;
            float dx = p2->pos_x - p1->pos_x;
            float dy = p2->pos_y - p1->pos_y;
            float dr = hypotf(dx,dy);
            if (dr == 0.0f) continue;
            force_curve * fc = types[p2->type].forces;
            float force = get_force(fc,dr);
            p1->vel_x += force*(dx/dr)*PP_FORCE_MULTIPLIER;
            p1->vel_y += force*(dy/dr)*PP_FORCE_MULTIPLIER;

        }
    }
    // Potential Forces and Positional Updates
    for (PARTICLES(p)){
        int x = (int) p->pos_x;
        int y = (int) p->pos_y;

        float xx = (x - mx/2)/(1.0*mx/2);
        float yy = (y - my/2)/(1.0*my/2);
        //0float rr = hypotf(xx,yy);
        float rr = sqrtf(sqrtf(xx*xx*xx*xx+yy*yy*yy*yy));

        rr /= POTENTIAL_SIZE;

        if (USE_PULSATION){
            rr /= 0.8+slow_phase;
        }

        if (USE_POTENTIAL){
            p->vel_x += -rr*rr*rr*rr*rr*rr*xx/100;
            p->vel_y += -rr*rr*rr*rr*rr*rr*yy/(100*POTENTIAL_ECCENTRICITY);
        }
        
        if (USE_ROTATION){
            p->vel_x += yy/100;
            p->vel_y += -xx/100;
        }

        if (USE_WIGGLE){
            p->vel_x += p->vel_y*fast_phase/5;
            p->vel_y += -p->vel_x*fast_phase/5;

        }

        if (USE_DAMPENING||USE_POTENTIAL_DAMPENING||USE_PULSATION_DAMPENING){
            if (USE_POTENTIAL_DAMPENING){
                float factor = rr*rr*rr*rr/1000;
                p->vel_x *= 1-(1-DAMPENING_CONSTANT)*factor;
                p->vel_y *= 1-(1-DAMPENING_CONSTANT)*factor;
            }else if (USE_PULSATION_DAMPENING) {
                p->vel_x *= 1-(1-DAMPENING_CONSTANT)*slow_phase;
                p->vel_y *= 1-(1-DAMPENING_CONSTANT)*slow_phase;
            } else {
                p->vel_x *= DAMPENING_CONSTANT;
                p->vel_y *= DAMPENING_CONSTANT;
            }
        }

        if (USE_STOKES_FRICTION){
            float v = hypotf(p->vel_x,p->vel_y);
            if (v > 1){
                p->vel_x -= (p->vel_x / v) * v*v/10;
                p->vel_y -= (p->vel_y / v) * v*v/10;
            }
        }
        

        p->pos_x += p->vel_x;
        p->pos_y += p->vel_y;
    }
}

void reset(void) {
	nexttick = udate();
	matrix_clear();
	randomize();
	frame = 0;
}

int draw(int argc, char* argv[]) {
    
    if (USE_TRAILS){
        for (int x = 0;x < matrix_getx();x++){
            for (int y = 0;y < matrix_gety();y++){
                RGB color;
                color = matrix_get(x,y);
                color.red = color.red * TRAIL_LENGTH/256;
                color.green = color.green * TRAIL_LENGTH/256;
                color.blue = color.blue * TRAIL_LENGTH/256;
                matrix_set(x,y,color);
            }
        }
    } else {
        matrix_clear();
    }
    

	update();
    if (isnan(particles->pos_x)) return 1; // TODO FIXME: better solution than that!
    
	for (particle * p = particles; p < particles + numparticles; ++p){
        int x = (int) p->pos_x;
        int y = (int) p->pos_y;
        if (!(x >= 0 && x < matrix_getx() && y >= 0 && y < matrix_gety())) continue;
        RGB color = RGB(255,255,255);
        if (USE_COLOR) color = p->color;
        if (USE_ADDITIVE_COLOR){
            RGB c = matrix_get(x,y);
            c.red = _min(c.red +color.red/5,255);
            c.green = _min(c.green + color.green/5,255);
            c.blue = _min(c.blue + color.blue/5,255);
            matrix_set(x,y,c);
        } else {
            mset(x,y,color);
        }
    }

    for (int i = 0;i<numtypes;i++){
        for (int j = 0; j<numtypes;j++){

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

int deinit() {
	free(particles);
    for (particle_type * t = types;t<types+numtypes;t++){
        free(t->forces);
    }
    free(types);
	return 0;
}
