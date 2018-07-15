// ASL: Advanced String Lib
// ... what did you expect it to be?

#ifndef __INCLUDED_ASL__
#define __INCLUDED_ASL__

#include <stdint.h>

// Fast (and questionable) comparisons of 4 char strings.
#define ASL_4CPTR2N(str) *((int32_t*) ((char*) (str)))
#define ASL_COMP4C(str1, str2) (ASL_4CPTR2N((str1)) == ASL_4CPTR2N((str2)))

// Adds a character to a string, and disposes of the old one, unless it's NULL (in which case this creates a new one-char string)
// Can return NULL itself on malloc failure (in which case the original is still freed)
extern char * asl_growstr(char * str, char nxt);

// Adds an argument to an argv, and disposes of the old one, unless it's NULL.
// Element pointers are copied (no strdups).
// Can return NULL itself on malloc failure (in which case everything is freed)
extern char ** asl_growav(int argc, char ** argv, char * nxt);

// Like the previous, but prepending.
extern char ** asl_pgrowav(int argc, char ** argv, char * nxt);

// Like the previous, but removing the first, not adding to it.
// It's assumed you've "collected" it elsewhere.
extern char ** asl_pnabav(int argc, char ** argv);

// If argv != NULL: all args will be freed if relevant, then argv will be freed
extern void asl_free_argv(int argc, char ** argv);

#endif
