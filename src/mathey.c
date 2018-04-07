// Mathey, tiny math helpers.
// Why? Cause they're handy.
// Why not macros? Cause smaller binary size.

#include "types.h"
#include <math.h>

#define sign(x) ((x < 0) ? -1 : ( (x > 0) ? 1 : 0))

byte bdiff(byte a, byte b) {
	if (a > b) return a - b;
	if (a < b) return b - a;
	return 0;
}

byte bmin(byte a, byte b) {
	return (a > b) ? a : b;
}

byte bmax(byte a, byte b) {
	return (a < b) ? a : b;
}

// Matrix/Vector stuff
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

vec2 vadd(vec2 v1, vec2 v2) {
	vec2 r = {
		.x = v1.x + v2.x,
		.y = v1.y + v2.y,
	};
	return r;
}

vec2 vmul(vec2 vec, float val) {
	vec2 r = {
		.x = vec.x * val,
		.y = vec.y * val,
	};
	return r;
}

vec2 vdiv(vec2 v1, vec2 v2) {
	vec2 r = {
		.x = v1.x / v2.x,
		.y = v1.y / v2.y,
	};
	return r;
}

matrix mmult(matrix m1, matrix m2) {
	matrix r = {
		.v1_1 = (m1.v1_1 * m2.v1_1) + (m1.v1_2 * m2.v2_1),
		.v1_2 = (m1.v1_1 * m2.v1_2) + (m1.v1_2 * m2.v2_2),
		.v2_1 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
		.v2_2 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
	};
	return r;
}

vec2 vmmult(matrix m, vec2 v) {
	vec2 r = {
		.x = (m.v1_1 * v.x) + (m.v1_2 * v.y),
		.y = (m.v2_1 * v.x) + (m.v2_2 * v.y),
	};
	return r;
}
