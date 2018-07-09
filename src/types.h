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
	byte alpha;
} RGB;

typedef struct HSV {
	byte h;
	byte s;
	byte v;
	byte pad; // yay, more padding.
} HSV;

extern RGB HSV2RGB(HSV hsv);
extern HSV RGB2HSV(RGB rgb);
extern RGB RGBlerp(byte v, RGB rgbA, RGB rgbB);

// Macro for painless colors.
#define RGB_C(r, g, b) ((RGB) { .red = (byte) (r), .green = (byte) (g), .blue = (byte) (b), .alpha = 255 } )
#define HSV_C(hue, sat, val) ((HSV) { .h = (byte) (hue), .s = (byte) (sat), .v = (byte) (val), .pad = 0 } )

#define RGB(r, g, b) RGB_C(r, g, b)
#define HSV(h, s, v) HSV_C(h, s, v)

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
