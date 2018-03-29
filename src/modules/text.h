// Text stuff.
#include <types.h>

typedef struct text {
	int len;
	byte* buffer;
} text;

// Get if the bit is set or not at a position.
// Font is 8px tall at max. Proportional.
int text_point(text* rendered, int x, int y);
// Renders text.
// Returns heap allocated pointer or NULL if something failed.
text* text_render(const char* txt);
// Frees rendered text. Do it. Don't forget to, leaks a bad, mmkay?
// No harm in calling it multiple times.
void text_free(const text* rendered);
