#include "types.h"
#include <math.h>
#include <stdarg.h>

#define sign(x) ((x<0)?-1:((x>0)?1:0))

extern byte bdiff(byte a, byte b);
extern byte bmin(byte a, byte b);
extern byte bmax(byte a, byte b);

float square(float x);

typedef struct vec2 {
	float x;
	float y;
} vec2;

typedef struct matrix2_2 {
	float v1_1;
	float v1_2;
	float v2_1;
	float v2_2;
} matrix2_2;

typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3;

typedef struct matrix3_3 {
  float v1_1;
  float v1_2;
  float v1_3;
  float v2_1;
  float v2_2;
  float v2_3;
  float v3_1;
  float v3_2;
  float v3_3;
} matrix3_3;

#define vec2(xv, yv) ((vec2) { .x = (xv), .y = (yv)})
#define vec3(xv, yv, zv) ((vec3) { .x = (xv), .y = (yv), .z = (zv)})

vec2 vadd(vec2 v1, vec2 v2);
vec2 vmul(vec2 v1, float val);
vec2 vdiv(vec2 v1, vec2 v2);
vec2 multm2v2(matrix2_2 m, vec2 v);
matrix2_2 multm2(matrix2_2 m1, matrix2_2 m2);
matrix3_3 multm3(matrix3_3 m1, matrix3_3 m2);
matrix3_3 identity3();
matrix3_3 composem3(int n, ...);
matrix3_3 rotation3(float angle);
matrix3_3 translation3(float x, float y);
matrix3_3 translation3_v2(vec2 v);
matrix3_3 translation3_v3(vec3 v);
matrix3_3 scale3(float x_factor, float y_factor);
matrix3_3 shear3(float x_shear, float y_shear);
vec3 vec2tovec3(vec2 v);
vec2 vec3tovec2(vec3 a);
vec3 multm3v3(matrix3_3 m, vec3 v);
vec2 multm3v2(matrix3_3 m, vec2 v);

static inline vec2 multm3v2_partx(matrix3_3 m, float v_x) {
  vec2 r = {
      .x = (m.v1_1*v_x) + (m.v1_3),
      .y = (m.v2_1*v_x) + (m.v2_3)
  };
  return r;
}

static inline vec2 multm3v2_partxy(matrix3_3 m, vec2 kern_x, float v_y) {
  vec2 r = {
      .x = kern_x.x + (m.v1_2*v_y),
      .y = kern_x.y + (m.v2_2*v_y)
  };
  return r;
}
