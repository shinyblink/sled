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

static float sx, sy, t;
typedef struct spectrum {
    float r;
    float g;
    float b;
} spectrum;
#define SPEC(pr, pg, pb) ((spectrum){.r = (float) (pr), .g = (float) (pg), .b = (float) (pb)})

static float * intensities;
static float * rnd;


int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    intensities = malloc(sizeof(int)*mx*my);
    rnd = malloc(sizeof(int)*mx*my);
    modno = moduleno;
    frame = 0;
    return 0;
}

static RGB intense_red(float p1){
    int intensity = p1*511;
    if (intensity < 0) intensity = 0;
    if (intensity > 511) intensity = 511;
    if (intensity < 256) {
        return RGB(intensity/2,0,intensity/2);
    } else {
        return RGB(255,intensity-256,intensity/2);
    }
}
static inline RGB CDB(float v){
    //printf("%f\n",v);
    v = v;
    if (v >= 0 && v < 1.0) return RGB(255*v,255*v,255*v); // white (0,1)
    if (v <= 0 && v > -1.0) return RGB(255*(-v),0,0); // red (-1,0)
    if (v >= 1.0) return RGB(255*v-255,128*v-128,0); // (1,2) orange -> green -> yellow
    if (v <= -1.0) return RGB(0,128*(-v)-128,255*(-v)-255); // purple < -1
    return RGB(255,0,255);
}

void reset(int _modno) {
    for (int i=0;i<mx*my;i++){
        intensities[i] = randn(1000)/1000.0f;
        rnd[i] = randn(1000)/1000.0f;
    }
    nexttick = udate();
    frame = 0;
}

#define ATMO_NORMAL(x) x+asin(cos(x)/r_athmos)
#define lerp(a,b,c) ((c)*(a)+(1-(c))*(b))
//#define sigm(a) (1./(1.+expf(-((float)a))))
#define sigm(a) (.5+.5*tanh(.5*(a)))
#define fade(a,b,c,d) lerp(a,b,sigm(c-d))
#define lerpspec(A,B,c) (SPEC(lerp(A.r,B.r,c),lerp(A.g,B.g,c),lerp(A.b,B.b,c)))
#define maxat(a,b,c) (exp(-((a)-(b))*((a)-(b))*(c)*(c)))
/* Law of sines:
 * sin(alpha)/a = sin(beta)/b = sin(gamma) / c
 */


static spectrum dispersion(float dy){
    // dy = altitude above horizon
    //

}

static spectrum domain_space(float dx, float dy, int xx, int yy){
    float horizontal = dx-sx;
    float deflection = dy-sy;
    float direct = horizontal*horizontal+deflection*deflection;
    float scatter_near = expf(-direct*400);
    //return SPEC(1,1,1);
    return SPEC(scatter_near,scatter_near,scatter_near);
    if (scatter_near > 0.9) return SPEC(1,1,1);
    return SPEC(0,0,0);
}

static spectrum ray_scatter(float dx, float dy, int xx, int yy){
    float horizontal = dx-sx;
    float deflection = dy-sy;
    float direct = horizontal*horizontal+deflection*deflection;
    float scatter_far = expf(-direct*10);
    float r = expf(-direct*8)*0.5;
    float g = expf(-direct*5)*0.7;
    float b = expf(-direct);
    return SPEC(r,g,b);

    return SPEC(0,0,1);
}

static spectrum filter_direct(spectrum in,float dy){
    //float r = in.r * lerp(1,1,sigm(10*(dy-0.2)));
    float r = in.r;
    float g = in.g * lerp(1,0.05,sigm(10*(dy-0.5)));
    float b = in.b * lerp(1,0.02,sigm(10*(dy-0.7)));
    return SPEC(r,g,b);
}


static spectrum domain_sky(float dx, float dy, int xx, int yy){
    spectrum scatter = ray_scatter(dx, dy, xx, yy);
    spectrum sun = domain_space(dx, dy, xx, yy);
    //sun = filter_direct(sun,dy);
    //return sun;
    //sun = SPEC(0,0,0);
    spectrum mix = lerpspec(scatter,sun,lerp(0.2,0.5,t));
    //mix = filter_direct(mix,dy);
    return mix;
}

static spectrum hit_ground(float dx, float dy, int xx, int yy){
    float r1 = intensities[xx+mx*(yy/3)];
    float r2 = intensities[xx+mx*yy]+t*0.007;
    if (xx%2 && yy%2 && 0.4 < r2 && r2 < 0.45) return SPEC(r2+0.1*r1,r2-0.1*r1,0);
    float v = (r1/20)*(1-t);
    return SPEC(v,v,v);
}

static spectrum domain_ground(float dx, float dy, int xx, int yy){
    float curvature = -0.1*(1-dx*dx);
    //curvature = 0;
    float horizon_obs = curvature-intensities[xx+3*mx]*0.025;
    //struct spectrum = {.r=0,.g=0,.b=0};

    if (horizon_obs+dy <= 0)
        return hit_ground(dx, dy-curvature, xx, yy);
    return domain_sky(dx, dy, xx, yy);

}

static inline RGB ray(int xx, int yy){
    float dx = (float)xx/(float)mx-0.5;
    float dy = 1.0-(float)(yy+1)/(float)my;

    spectrum spec = domain_ground(dx, dy, xx, yy);
    return RGB(spec.r*255,spec.g*255,spec.b*255);
}

/*
static inline RGB calc(int xx, int yy, float t, int i){
    float angle = atan2f(y+1.0/my,x)/M_PI;
    //if (!randn(1000)) printf("%f %f %f %f\n",sx,sy,x,y);
    float r=1,g=1,b=1;


    float horizontal = (x-sx)*M_PI/4; // horizontal deflection
    float view = (y+curvature)*M_PI/4; // 0 < _ < pi/4 //  angle above horizon (between 0 and 45deg /_ )
    float sun = (sy+curvature)*M_PI/4; // +- pi /2  // sun angle about 30deg and falling

    float r_athmos = 1.0001; // r > 1
    float athmosphere = ATMO_NORMAL(view); // atmosphere normal angle
    float athmosphere_max = ATMO_NORMAL(M_PI/4);
    float athmosphere_min = ATMO_NORMAL(0);
    // athmosphere angle:
    // r -> 1: min -> pi/2, max -> pi/2
    // r < 1.001 (small)" 70deg < athmosphere normal  < 90 deg
    // r = 2: 30deg < normal < 66deg
    // r -> inf: min -> 0deg , max -> 45deg
    float diffraction = M_PI/2 - (athmosphere - sun); // light ray angle change

    // athmosphere path 
    // sin(athmosphere)/path = cos(view)/r_athmos
    float path_length_direct = sinf(athmosphere)/cosf(view)*(r_athmos-1);
    float path_length_extra = athmosphere - (sun-M_PI/2);
    float path_length = path_length_direct+(path_length_extra*sigm(-diffraction*5));
    float relative_path = (path_length_direct/ (r_athmos-1));
    // PATH PENALTY
    //r *= lerp(1,0.9,sigm((0.3-relative_path*1.1)*2));
    //b *= lerp(1,0.1,sigm((0.3-relative_path*1.1)*2));
    //g *= lerp(1,0.05,sigm((0.45-relative_path*1.1)*1.8));

    //return sigm((dy-0.5)*20);
    //float something = (diffraction);

    float deflection = sun - view; // +- pi // light deflection angle

    float direct = horizontal*horizontal+deflection*deflection;
    float scatter_near = expf(-direct*400);
    float scatter_mid = expf(-direct*10);
    float scatter_far = expf(-direct*1);

    // PATH PENALTY
    r *= lerp(1,0,sigm(relative_path));
    //r *= lerp(1,0.9,sigm((0.3-relative_path*1.1)*2));
    //b *= lerp(1,0.1,sigm((0.3-relative_path*1.1)*2));
    //g *= lerp(1,0.05,sigm((0.45-relative_path*1.1)*1.8));

    // SCATTER
    //r *= lerp(1,0,scatter_far);
    //g *= lerp(1,0,scatter_far);
    //b *= lerp(1,0,scatter_far);
    //r *= lerp(1,1,scatter_mid);
    //g *= lerp(1,1,scatter_mid);
    //b *= lerp(1,1,scatter_mid);
    //r *= lerp(1,1,scatter_near);
    //g *= lerp(1,1,scatter_near);
    //b *= lerp(1,1,scatter_near);



    //if (xx == yy) printf("%f\n",relative_path);
    //r *= lerp(1,0,sigm((1.1-relative_path*1.1)*0.8));
    //g *= lerp(1,0,sigm(1.5-relative_path));
    if (horizon_obs+y <= 0) return RGB(0,0,0);

    //return (xx %2) ? CDB(r):CDB(g) ;
    return CDB(r);
    return RGB(r*255,g*255,b*255);
}
*/


int draw(int _modno, int argc, char* argv[]) {
    t = (float)frame/(1+(float)FRAMES);
    if (t > 1.0) t = 1.0;
    sx = t*(0.4+0.1*rnd[0])-0.2-.1*rnd[1];
    sy = 0.6+.1*rnd[2]-t*(1.1+0.1*rnd[3]);
    for (int x = 0; x<mx; x++) {
        for (int y=0; y<my; y++) {
            int i = x + mx*y;
            RGB res = ray(x,y);
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
    free(rnd);
}
