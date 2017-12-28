// Mathey, tiny math helpers.
// Why? Cause they're handy.
// Why not macros? Cause smaller binary size.

#include <types.h>
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
};
