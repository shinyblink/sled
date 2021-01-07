#pragma once
#ifndef MATHEY_H
#define MATHEY_H

#include "types.h"
#include <math.h>
#include <stdarg.h>

#define sign(x) ((x<0)?-1:((x>0)?1:0))

extern byte bdiff(byte a, byte b);
extern byte bmin(byte a, byte b);
extern byte bmax(byte a, byte b);

float square(float x);

/** 
 * Plain 2D vector.
 */
typedef struct vec2 {
	float x;
	float y;
} vec2;

/**
 * Plain 2D matrix
 */
typedef struct matrix2_2 {
	float v1_1;
	float v1_2;
	float v2_1;
	float v2_2;
} matrix2_2;

/**
 * Plain 3D vector _OR_ homogenous 2D vector (if .z == 1)
 */
typedef struct vec3 {
  float x;
  float y;
  float z;
} vec3;

/**
 * Plain 3D matrix _OR_ homogenous 2D transformation matrix (if last row == {0, 0, 1})
  */
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

/**
 * Homogenous 3D vector (with .w == 1)
 */
typedef struct vec4 {
  float x;
  float y;
  float z;
  float w;
} vec4;

/**
 * Homogenous 3D transformation matrix (with last row == {0,0,0,1})
 */
typedef struct matrix4_4 {
  float v1_1;
  float v1_2;
  float v1_3;
  float v1_4;
  float v2_1;
  float v2_2;
  float v2_3;
  float v2_4;
  float v3_1;
  float v3_2;
  float v3_3;
  float v3_4;
  float v4_1;
  float v4_2;
  float v4_3;
  float v4_4;
} matrix4_4;

/**
 * Vector initialization macros.
 */
#define vec2(xv, yv) ((vec2) { .x = (xv), .y = (yv)})
#define vec3(xv, yv, zv) ((vec3) { .x = (xv), .y = (yv), .z = (zv)})
#define vec4(xv, yv, zv, wv) ((vec4) { .x = (xv), .y = (yv), .z = (zv), .w = (wv)})

/**
 * Add two plain 2D vectors
 */
vec2 vadd(vec2 v1, vec2 v2);

/**
 * Multiply a plain 2D vector with a value ("scale")
 */
vec2 vmul(vec2 v1, float val);

/**
 * Add two plain 3D vectors (don't add homogenous 2D vectors with this, use a translation matrix instead!)
 */
vec3 v3add(vec3 v1, vec3 v2);

/**
 * Multiply a matrix with a vector (component-based scaling). 
 */
vec2 multm2v2(matrix2_2 m, vec2 v);
vec3 multm3v3(matrix3_3 m, vec3 v);
vec2 multm3v2(matrix3_3 m, vec2 v);
vec4 multm4v4(matrix4_4 m, vec4 v);
vec3 multm4v3(matrix4_4 m, vec3 v);

/**
 * Multiply two matrices of same size ("compose").
 */
matrix2_2 multm2(matrix2_2 m1, matrix2_2 m2);
matrix3_3 multm3(matrix3_3 m1, matrix3_3 m2);
matrix4_4 multm4(matrix4_4 m1, matrix4_4 m2);

/**
 * Returns a 3D identity matrix
 */
matrix3_3 identity3();

/**
 * Composes (multiplies) n 3D/homogenous 2D matrices
 */
matrix3_3 composem3(int n, ...);


/**
 * Returns a homogenous 2D rotation matrix
 */
matrix3_3 rotation3(float angle);

/**
 * Returns a homogenous 2D translation matrix
 */
matrix3_3 translation3(float x, float y);
matrix3_3 translation3_v2(vec2 v);
matrix3_3 translation3_v3(vec3 v);

/**
 * Returns a homogenous 2D scale matrix
 */
matrix3_3 scale3(float x_factor, float y_factor);

/**
 * Returns a homogenous 2D shear matrix
 */
matrix3_3 shear3(float x_shear, float y_shear);


/**
 * Returns a homogenous 3D identity matrix
 */
matrix4_4 identity4();

/**
 * Composes (multiplies) n homogenous 3D identity matrices
 */
matrix4_4 composem4(int n, ...);

/**
 * Returns a homogenous 3D rotation matrix around the x-axis
 */
matrix4_4 rotation4x(float angle);

/**
 * Returns a homogenous 3D rotation matrix around the y-axis
 */
matrix4_4 rotation4y(float angle);

/**
 * Returns a homogenous 3D rotation matrix around the z-axis
 */
matrix4_4 rotation4z(float angle);

/**
 * Returns a homogenous 3D rotation matrix representing a rotation around the x-axis, then y-axis, then z-axis
 */
matrix4_4 rotation4xyz(float x_angle, float y_angle, float z_angle);

/**
 * Returns a homogenous 3D rotation matrix representing a rotation around the z-axis, then y-axis, then x-axis
 */
matrix4_4 rotation4zyx(float x_angle, float y_angle, float z_angle);

/**
 * Returns a homogenous 3D translation matrix
 */
matrix4_4 translation4(float x, float y, float z);
matrix4_4 translation4_v3(vec3 v);
matrix4_4 translation4_v4(vec4 v);

/**
 * Returns a homogenous 3D scaling matrix
 */
matrix4_4 scale4(float x_factor, float y_factor, float z_factor);

/**
 * Returns a homogenous 3D shearing matrix
 */
matrix4_4 shear4(float xy_shear, float xz_shear, float yx_shear, float yz_shear, float zx_shear, float zy_shear);


/**
 * Converts between plain vectors and homogenous vectors
 */
vec4 vec3tovec4(vec3 v);
vec3 vec4tovec3(vec4 a);
vec3 vec2tovec3(vec2 v);
vec2 vec3tovec2(vec3 a);

/**
 * This is a split-up version of multm3v2: if used correctly, this saves cycles.
 *
 * multm3v2_partx calculates only the x-axis-dependent parts of a multiplication of a matrix with a vector (the "x kernel").
 * multm3v2_partxy calculates only the y-axis-dependent parts, using the precalculated "x kernel".
 *
 * This is usable in the following situation, where the x-axis-dependent part is calculated again and again
 * for each point on the y axis:
 *
 * for(int x = 0; x < MATRIX_X; x++) {
 *   for(int y = 0; y < MATRIX_Y; y++) {
 *     vec2 point = multm3v2(my_composed_transformation_matrix, vec2(x,y));
 *   }
 * }
 *
 * The point here is: All operations on x are repeated for each y, even though x does not change inside the y loop.
 * To remedy this, the multm3v2 operation can be split up, leading to the following code:
 *
 * for(int x = 0; x < MATRIX_X; x++) {
 *   vec2 x_kernel = multm3v2_partx(my_composed_transformation_matrix, x);
 *   for(int y = 0; y < MATRIX_Y; y++) {
 *     vec2 point = multm3v2_partxy(my_composed_transformation_matrix, x_kernel, y);
 *   }
 * }
 *
 */
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

#endif