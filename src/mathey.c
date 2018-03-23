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
typedef struct vector {
	float x;
	float y;
} vector;

typedef struct matrix {
	float v1_1;
	float v1_2;
	float v2_1;
	float v2_2;
} matrix;

vector vadd(vector v1, vector v2) {
	vector r = {
		.x = v1.x + v2.x,
		.y = v1.y + v2.y,
	};
	return r;
};

matrix mmult(matrix m1, matrix m2) {
	matrix r = {
		.v1_1 = (m1.v1_1 * m2.v1_1) + (m1.v1_2 * m2.v2_1),
		.v1_2 = (m1.v1_1 * m2.v1_2) + (m1.v1_2 * m2.v2_2),
		.v2_1 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
		.v2_2 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
	};
	return r;
};

vector vmmult(matrix m, vector v) {
	vector r = {
		.x = (m.v1_1 * v.x) + (m.v1_2 * v.y),
		.y = (m.v2_1 * v.x) + (m.v2_2 * v.y),
	};
	return r;
};
