#include "types.h"
#include <math.h>

#define sign(x) ((x<0)?-1:((x>0)?1:0))

extern byte bdiff(byte a, byte b);
extern byte bmin(byte a, byte b);
extern byte bmax(byte a, byte b);

typedef struct vec2 {
	float x;
	float y;
} vec2;

typedef struct matrix {
	float v1_1;
	float v1_2;
	float v2_1;
	float v2_2;
} matrix;

#define vec2(xv, yv) ((vec2) { .x = (xv), .y = (yv)})

vec2 vadd(vec2 v1, vec2 v2);
vec2 vmul(vec2 v1, vec2 v2);
vec2 vdiv(vec2 v1, vec2 v2);
matrix mmult(matrix m1, matrix m2);
vec2 vmmult(matrix m, vec2 v);
