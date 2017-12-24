// Utilties provided by main.c for control of the main loop.

// Changes the outcome of the next random module selection.
// If the outcome has already been changed, waits for that to occur, then tries to grab the random module selection again.
// Note that this should only be called from another thread
//  (as it's only really useful there anyway, but also because it can freeze if called from the main thread)
// If a given list of forced randoms occur from a single thread in order,
//  the results will be ordered and the thread will do a lot of sleeping.
// (FISh uses this for the "/then" command.)
extern void main_force_random(int moduleno, int argc, char ** argv);
