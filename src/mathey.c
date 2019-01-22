// Mathey, tiny math helpers.
// Why? Cause they're handy.
// Why not macros? Cause smaller binary size.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
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

#include "types.h"
#include <math.h>
#include <stdarg.h>

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

inline float square(float x) {
  return x*x;
}

// Matrix/Vector stuff
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

vec2 multm2v2(matrix2_2 m, vec2 v) {
	vec2 r = {
		.x = (m.v1_1 * v.x) + (m.v1_2 * v.y),
		.y = (m.v2_1 * v.x) + (m.v2_2 * v.y),
	};
	return r;
}

matrix2_2 multm2(matrix2_2 m1, matrix2_2 m2) {
	matrix2_2 r = {
		.v1_1 = (m1.v1_1 * m2.v1_1) + (m1.v1_2 * m2.v2_1),
		.v1_2 = (m1.v1_1 * m2.v1_2) + (m1.v1_2 * m2.v2_2),
		.v2_1 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
		.v2_2 = (m1.v2_1 * m2.v1_1) + (m1.v2_2 * m2.v2_1),
	};
	return r;
}

matrix3_3 multm3(matrix3_3 m1, matrix3_3 m2) {
  matrix3_3 r = {
      .v1_1 = m1.v1_1*m2.v1_1 + m1.v1_2*m2.v2_1 + m1.v1_3*m2.v3_1,
      .v1_2 = m1.v1_1*m2.v1_2 + m1.v1_2*m2.v2_2 + m1.v1_3*m2.v3_2,
      .v1_3 = m1.v1_1*m2.v1_3 + m1.v1_2*m2.v2_3 + m1.v1_3*m2.v3_3,

      .v2_1 = m1.v2_1*m2.v1_1 + m1.v2_2*m2.v2_1 + m1.v2_3*m2.v3_1,
      .v2_2 = m1.v2_1*m2.v1_2 + m1.v2_2*m2.v2_2 + m1.v2_3*m2.v3_2,
      .v2_3 = m1.v2_1*m2.v1_3 + m1.v2_2*m2.v2_3 + m1.v2_3*m2.v3_3,

      .v3_1 = m1.v3_1*m2.v1_1 + m1.v3_2*m2.v2_1 + m1.v3_3*m2.v3_1,
      .v3_2 = m1.v3_1*m2.v1_2 + m1.v3_2*m2.v2_2 + m1.v3_3*m2.v3_2,
      .v3_3 = m1.v3_1*m2.v1_3 + m1.v3_2*m2.v2_3 + m1.v3_3*m2.v3_3
    };
  return r;
};

matrix3_3 identity3() {
  matrix3_3 r = {
    .v1_1 = 1,
    .v1_2 = 0,
    .v1_3 = 0,
    .v2_1 = 0,
    .v2_2 = 1,
    .v2_3 = 0,
    .v3_1 = 0,
    .v3_2 = 0,
    .v3_3 = 1
  };
  return r;
}

matrix3_3 composem3(int n, ...) {
  va_list valist;
  va_start(valist, n);

  matrix3_3 r = identity3();
  for (int i = 0; i < n; i++) {
    matrix3_3 x = va_arg(valist, matrix3_3);
    r = multm3(r,x);
  }

  va_end(valist);
  return r;
}

matrix3_3 rotation3(float angle) {
  matrix3_3 r = {
    .v1_1 = cos(angle),
    .v1_2 = -sin(angle),
    .v1_3 = 0,
    .v2_1 = sin(angle),
    .v2_2 = cos(angle),
    .v2_3 = 0,
    .v3_1 = 0,
    .v3_2 = 0,
    .v3_3 = 1
  };
  return r;
}

matrix3_3 translation3(float x, float y) {
  matrix3_3 r = {
    .v1_1 = 1,
    .v1_2 = 0,
    .v1_3 = x,
    .v2_1 = 0,
    .v2_2 = 1,
    .v2_3 = y,
    .v3_1 = 0,
    .v3_2 = 0,
    .v3_3 = 1
  };
  return r;
}

matrix3_3 translation3_v2(vec2 v) {
  return translation3(v.x, v.y);
}

matrix3_3 translation3_v3(vec3 v) {
  return translation3(v.x, v.y);
}

matrix3_3 scale3(float x_factor, float y_factor) {
  matrix3_3 r = {
    .v1_1 = x_factor,
    .v1_2 = 0,
    .v1_3 = 0,
    .v2_1 = 0,
    .v2_2 = y_factor,
    .v2_3 = 0,
    .v3_1 = 0,
    .v3_2 = 0,
    .v3_3 = 1
  };
  return r;
}

matrix3_3 shear3(float x_shear, float y_shear) {
  matrix3_3 r = {
    .v1_1 = 1,
    .v1_2 = x_shear,
    .v1_3 = 0,
    .v2_1 = y_shear,
    .v2_2 = 1,
    .v2_3 = 0,
    .v3_1 = 0,
    .v3_2 = 0,
    .v3_3 = 1
  };
  return r;
}

vec3 vec2tovec3(vec2 v) {
  vec3 a = {
    .x = v.x,
    .y = v.y,
    .z = 1
  };
  return a;
}

vec2 vec3tovec2(vec3 a) {
  vec2 v = {
    .x = a.x,
    .y = a.y
  };
  return v;
}

vec3 multm3v3(matrix3_3 m, vec3 v) {
  vec3 r = {
      .x = (m.v1_1*v.x) + (m.v1_2*v.y) + (m.v1_3*v.z),
      .y = (m.v2_1*v.x) + (m.v2_2*v.y) + (m.v2_3*v.z),
      .z = (m.v3_1*v.x) + (m.v3_2*v.y) + (m.v3_3*v.z)
  };
  return r;
}

vec2 multm3v2(matrix3_3 m, vec2 v) {
  vec2 r = {
      .x = (m.v1_1*v.x) + (m.v1_2*v.y) + (m.v1_3),
      .y = (m.v2_1*v.x) + (m.v2_2*v.y) + (m.v2_3)
  };
  return r;
}

