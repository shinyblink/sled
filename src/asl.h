// ASL: Advanced String Lib
// ... what did you expect it to be?

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

