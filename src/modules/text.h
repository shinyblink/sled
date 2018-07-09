// Text stuff.
#include <types.h>

typedef struct text_column {
	byte data[8];
} text_column;

typedef struct text {
	// Length of the buffer in columns.
	// Each column is 8 bytes.
	int len;
	text_column* buffer;
} text;

// Gets the lightness of a given pixel.
// Font is 8px tall at max. Proportional.
byte text_point(text* rendered, int x, int y);
// Renders text.
// Returns heap allocated pointer or NULL if something failed.
text* text_render(const char* txt);
// Frees rendered text. Do it. Don't forget to, leaks a bad, mmkay?
// Does nothing if called with NULL.
void text_free(text* rendered);
