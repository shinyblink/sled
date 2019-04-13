// ASL: Advanced String Lib
// ... what did you expect it to be?

#ifndef __INCLUDED_ASL__
#define __INCLUDED_ASL__

// Adds a character to a string, and disposes of the old one, unless it's NULL (in which case this creates a new one-char string)
// Can return NULL itself on malloc failure (in which case the original is still freed)
char * asl_growstr(char * str, char nxt);

// This structure SHOULD NOT be passed by pointer, and SHOULD NOT be malloc'd or free'd.
// asl_free_argv will only free the contents, not the structure itself.
typedef struct {
	int argc;
	char ** argv; // NULL for no elements.
} asl_av_t;

// Adds an argument to an argv, and disposes of the old one.
// Element pointers are copied (DOES NOT STRDUP!).
// On allocation error, will fail an assert.
// This is less flexible but simplifies involved code.
void asl_growav(asl_av_t * self, char * nxt);

// Like the previous, but prepending.
void asl_pgrowav(asl_av_t * self, char * nxt);

// Removes the first string. As such, ownership of that memory falls to you.
// Returns NULL if there's nothing to remove.
char * asl_pnabav(asl_av_t * self);

// Clears (frees all elements of, and the array of, then resets) an asl_av_t.
void asl_clearav(asl_av_t * self);

// Given a pair of asl_av_t * structures, find key in keys and use the index in vals.
// Returns NULL if the answer couldn't be found.
// This is not a particularly fast primitive and is meant to keep the SLED init code simple.
char * asl_getval(const char * key, asl_av_t * keys, asl_av_t * vals);
int asl_hasval(const char * key, asl_av_t * keys);

typedef struct {
	int argc;
	int * argv; // NULL for no elements.
} asl_iv_t;

void asl_growiv(asl_iv_t * self, int nxt);
void asl_pgrowiv(asl_iv_t * self, int nxt);
int asl_pnabiv(asl_iv_t * self);
void asl_cleariv(asl_iv_t * self);

// Test functions (use for tracing memory corruption)
void asl_test_av_validity(asl_av_t * self);
void asl_test_iv_validity(asl_iv_t * self);

#endif
