// Since we are all lazy bastards, here some aliases and macros.

#ifndef __INCLUDED_TYPES__
#define __INCLUDED_TYPES__

// Types
typedef unsigned char byte;

// Macros
#define ARRAY_SIZE(stuff) (sizeof(stuff) / sizeof(stuff[0]))
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

// Platform, since the below depends on it.

#if !defined(PLATFORM_RPI) && !defined(PLATFORM_DEBUG)
#error Must define platform: Define one of PLATFORM_{RPI,DEBUG}
#endif

#ifdef PLATFORM_DEBUG
// SDL-based platform.
#define COLOR_ORDER_RGB
#define MATRIX_ORDER_PLAIN
#undef COLOR_ORDER_GRB
#undef MATRIX_ORDER_SNAKE
#endif

// Colors
#if !defined(COLOR_ORDER_RGB) && !defined(COLOR_ORDER_GRB)
#define COLOR_ORDER_GRB // for now, neopixel/ws2812b style
#endif

#ifdef COLOR_ORDER_RGB
typedef struct RGB {
	byte red;
	byte green;
	byte blue;
} RGB;
#elif defined(COLOR_ORDER_GRB)
typedef struct RGB {
	byte green;
	byte red;
	byte blue;
} RGB;
#endif

// Matrix order and size

#if !defined(MATRIX_ORDER_PLAIN) && !defined(MATRIX_ORDER_SNAKE)
#define MATRIX_ORDER_SNAKE // cause that's what i have, it's also the easiest to wire, IMO.
#endif

#ifndef MATRIX_X
#error Define MATRIX_X as the matrixes X size.
#endif

#ifndef MATRIX_Y
#error Define MATRIX_Y as the matrixes Y size.
#endif

// Module and timer stuff.

#ifndef MAX_MODULES
#define MAX_MODULES 32
#endif

#endif
