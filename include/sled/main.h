// Utilties provided by main.c for control of the main loop.
#ifndef MAIN_H__
#define MAIN_H__

#include "mod.h"

// Changes the outcome of the next random module selection.
// If the outcome has already been changed, waits for that to occur, then tries to grab the random module selection again.
// Note that this should only be called from another thread
//  (as it's only really useful there anyway, but also because it can freeze if called from the main thread)
// If a given list of forced randoms occur from a single thread in order,
//  the results will be ordered and the thread will do a lot of sleeping.
// (FISh uses this for the "/then" command.)
extern void main_force_random(int moduleno, int argc, char ** argv);

// Our main method.
// Does the arg parsing and such.
// Since we might have a different main thing, we need to be called externally.
extern int sled_main(int argc, char** argv);

#endif
