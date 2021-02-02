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
#include <stdint.h>
#include "random.h"
#include <math.h>

#ifndef USE_STD_RAND
    #define RANDOM_RAW random_raw_global
    #define RAND_MAX 0xffffffff
#else
    #include <stdlib.h>
    #if RAND_MAX > 0x8000
        #define RANDOM_RAW std_rand_compatible_raw
        uint32_t std_rand_compatible_raw() {
            return rand() << 16 ^ rand();
        }
    #elif RAND_MAX > 0x80000000
        #define RANDOM_RAW rand
    #else
        #error using <stdlib> rand() has not enough entropy try undefining USE_STD_RAND
    #endif
#endif

#define NOISE_MAX 0xffffffff
#define UPPER_BIT 0x80000000

// https://www.wolframalpha.com/input/?i=random+number+between+0+and+2%5E32
#define RANDOM1 394442897
// https://www.wolframalpha.com/input/?i=random+prime+between+2%5E30+and+2%5E32
#define PRIME1 2426389093
#define PRIME2 2840406347

// Some nothing up my sleeve numbers
#define SLEEVE1 0x20078332 // Root Beer :P
#define SLEEVE2 0xcafebabe // Cafe Babe ;)
#define SLEEVE3 0xbaddecaf // bad Decaf x.x

typedef struct RNG_State {
    uint32_t counter;
    uint32_t jumper;
    uint32_t seed;
} RNG_State;

static RNG_State *global_rng_state = &((RNG_State) {SLEEVE1,SLEEVE2,SLEEVE3});


// https://en.wikipedia.org/wiki/Linear-feedback_shift_register
uint32_t lfsr(uint32_t value /*nonzero*/) {
    // https://users.ece.cmu.edu/~koopman/lfsr/index.html
    // https://users.ece.cmu.edu/~koopman/lfsr/32.txt
#define LFSR_FEED 80000967 // max cycle 32-bit
    return (value >> 1) ^ ((value & 1) ? LFSR_FEED : 0);
}

// https://en.wikipedia.org/wiki/Xorshift
uint32_t xorshift(uint32_t value /*nonzero*/) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

uint64_t xorshift64(uint64_t value) {
    value ^= value << 13;
    value ^= value >> 7;
    value ^= value << 17;
    return value;
}

// adapted from GDC 2017 "Math for game programmers - Noise-Based RNG"
uint32_t squirrel_like(uint32_t value, uint32_t seed) {
    value *= PRIME1;
    value += seed;
    value ^= (value << 13);
    value += RANDOM1;
    value ^= (value >> 17);
    value *= PRIME2;
    value ^= (value << 5);
    return value;
}

void random_seed(uint64_t seed) {
    seed = xorshift64(seed);
    global_rng_state->counter = seed;
    global_rng_state->jumper = SLEEVE2;
    global_rng_state->seed = squirrel_like(seed, seed>>32);
}

RNG_State advance_state(RNG_State* state) {
    return (RNG_State) {
        state->counter+1,lfsr(state->jumper),state->seed
    };
}


uint32_t random_raw_global() {
    *global_rng_state = advance_state(global_rng_state);
    return squirrel_like(global_rng_state->jumper^global_rng_state->counter,global_rng_state->seed);
}

uint32_t noise_raw(rand_t rand);
uint32_t random_raw(){return noise_raw(*RANDOM_RAW);}
uint32_t noise_raw(rand_t rand){
    return rand();
}

uint32_t noise_integer(uint32_t limit, rand_t rand);
uint32_t random_integer(uint32_t limit){ return noise_integer(limit, *RANDOM_RAW);}
uint32_t noise_integer(uint32_t limit, rand_t rand){
    if (limit <= 0) return 0;
    uint32_t r, max_r;
    max_r = NOISE_MAX-(NOISE_MAX%limit);
    do r = rand();
    while (r >= max_r);
    return r % limit;
}

uint32_t noise_integer_between(uint32_t lower, uint32_t upper, rand_t rand);
uint32_t random_integer_between(uint32_t lower, uint32_t upper){
    return noise_integer_between(lower, upper, *RANDOM_RAW);
}
uint32_t noise_integer_between(uint32_t lower, uint32_t upper, rand_t rand){
    uint32_t limit = upper-lower+1;
    if (limit <= 0) return 0;
    return noise_integer(limit,rand)+lower;
}


float noise_float_one(rand_t rand){
    uint32_t r = random_raw(rand);
    uint32_t exponent = 126; // biased exponent 0.5 - 1.0
    // find leading 1 bit
    while (r) {
        if (r&UPPER_BIT) break;
        exponent--;
        r = r << 1;
    }
    if (r == 0) return 0.0;
    union {
        float f;
        uint32_t i;
    } u;
    u.i = exponent << 23;
    r = r << 1; // hidden bit
    u.i |= r >> 9;
    return u.f;
}
float random_float_one(){return noise_float_one(*RANDOM_RAW);}

float noise_float_zero(rand_t rand){
    uint32_t r = random_raw(rand);
    uint32_t exponent = 126; // biased exponent 0.5 - 1.0
    // find leading 1 bit
    union {
        float f;
        uint32_t i;
    } u;
    u.i |= r & UPPER_BIT; // random sign
    r = r << 1; // consume bit
    while (r) {
        if (r&UPPER_BIT) break;
        exponent--;
        r = r << 1;
    }
    if (r == 0) return 0.0;
    u.i = exponent << 23;
    r = r << 1; // hidden bit
    u.i |= r >> 9;
    return u.f;
}
float random_float_zero(){return noise_float_zero(*RANDOM_RAW);}


float noise_float(float min, float max, rand_t rand);
float random_float(float min, float max){ return noise_float(min,max,*RANDOM_RAW);}
float noise_float(float min, float max, rand_t rand) {
    return min+noise_float_one(rand)*(max-min);
}

int noise_probability(float p, rand_t rand);
int random_probability(float p){ return noise_probability(p, *RANDOM_RAW);}
int noise_probability(float p, rand_t rand) {
    return noise_float_one(rand) < p;
}

two_normals_t noise_2_normals(rand_t rand);
two_normals_t random_2_normals(){ return noise_2_normals(*RANDOM_RAW);}
two_normals_t noise_2_normals(rand_t rand) {
    float f1, f2, r;
    do {
        f1 = noise_float_one(rand)*2-1;
        f2 = noise_float_one(rand)*2-1;
        r = f1*f1+f2*f2;
    } while (r > 1.0|| r <= 0.0);
    r = sqrtf((-2.0/r)*logf(r));
    return (two_normals_t) {f1*r,f2*r};
}

float noise_normal(rand_t rand) {
    two_normals_t noise = noise_2_normals(rand);
    return noise.x;
}
float random_normal(){ return noise_normal(*RANDOM_RAW);}


uint32_t noise_weighted_index(float * weights, unsigned n, rand_t rand);
uint32_t random_weighted_index(float * weights, unsigned n) {
    return noise_weighted_index(weights, n, *RANDOM_RAW);
}
uint32_t noise_weighted_index(float * weights, unsigned n, rand_t rand) {
    float sum = .0f;
    for (int i = 0; i < n; i++) {
        sum += weights[i];
    }
    float r = random_float_one()*sum;
    for (int i = 0; i < n; i++) {
        r -= weights[i];
        if (r <= 0) return i;
    }
    // some float precision errors or my fault
    // should be pretty low probability
    // but make this fair anyway
    return random_integer(n);
}

