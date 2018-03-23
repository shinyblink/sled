#include "types.h"
#include <math.h>

#define sign(x) ((x<0)?-1:((x>0)?1:0))

extern byte bdiff(byte a, byte b);
extern byte bmin(byte a, byte b);
extern byte bmax(byte a, byte b);

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

vector vadd(vector v1, vector v2);
matrix mmult(matrix m1, matrix m2);
vector vmmult(matrix m, vector v);
