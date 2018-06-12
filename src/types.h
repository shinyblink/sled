// Since we are all lazy bastards, here some aliases and macros.

#ifndef __INCLUDED_TYPES__
#define __INCLUDED_TYPES__

// Types
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;

#define T_MILLISECOND 1000ull
#define T_SECOND      1000000ull

// Macros
#define ARRAY_SIZE(stuff) (sizeof(stuff) / sizeof(stuff[0]))
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// Colors
typedef struct RGB {
	byte red;
	byte green;
	byte blue;
	byte pad; // uh. alpha?
} RGB;

typedef struct HSV {
	byte h;
	byte s;
	byte v;
	byte pad; // yay, more padding.
} HSV;

extern RGB HSV2RGB(HSV hsv);
extern HSV RGB2HSV(RGB rgb);

// Macro for painless colors.
#define RGB(r, g, b) ((RGB) { .red = (byte) (r), .green = (byte) (g), .blue = (byte) (b), .pad = 0 } )
#define HSV(hue, sat, val) ((HSV) { .h = (byte) (hue), .s = (byte) (sat), .v = (byte) (val), .pad = 0 } )

// Module and timer stuff.
#ifndef MAX_MODULES
#define MAX_MODULES 32
#endif

#ifndef MAX_TIMERS
#define MAX_TIMERS 256
#endif

#ifndef RANDOM_TIME // time in seconds until the core queues a random module.
#define RANDOM_TIME 5
#endif

#endif
