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


#ifndef __INCLUDE_NOISE__
#define __INCLUDE_NOISE__

#include <stdint.h>
#undef RAND_MAX
#define RAND_MAX 0xffffffff

// random number generator type
typedef uint32_t (*rand_t)(void);

//////////////////////
// Legacy interface //
//////////////////////

// 0 <= r <= limit
#define randn(limit) random_integer(limit+1)
extern void random_seed(void);

/////////////////////////////
// Generated Random Values //
/////////////////////////////

// P(1) = p | P(0) = 1-p
extern int random_probability(float p);
extern int noise_probability(float p,rand_t rand);

// 0 <= r <= 0xffffffff
extern uint32_t random_raw();
extern uint32_t noise_raw(rand_t rand);

// 0 <= r < limit
extern uint32_t random_integer(uint32_t limit);
extern uint32_t noise_integer(uint32_t limit,rand_t rand);

// lower <= r <= upper
extern uint32_t random_integer_between(uint32_t lower, uint32_t upper);
extern uint32_t noise_integer_between(uint32_t lower, uint32_t upper, rand_t rand);

// 0.0 <= r <= 1.0
extern float random_float_raw();
extern float noise_float_raw(rand_t rand);

// min <= r <= max
extern float random_float(float min, float max);
extern float noise_float(float min, float max, rand_t rand);

// P(i) = weights[i] | 0 <= i < n
// weights[i] can be positive / weights don't have to add to 1
extern uint32_t random_weighted_index(float * weights, unsigned n);
extern uint32_t noise_weighted_index(float * weights, unsigned n, rand_t rand);

// N(0,1) "normal distribution"
float random_normal();
float noise_normal(rand_t rand);
typedef struct {float x; float y;} two_normals_t;
extern two_normals_t random_2_normals();
extern two_normals_t noise_2_normals(rand_t rand);

#endif
