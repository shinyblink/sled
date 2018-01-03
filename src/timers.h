// Timers

#define SECOND 100000

typedef struct timer {
	int moduleno;
	unsigned long time;

	int argc;
	// Malloc'd data containing strdup/malloc'd data, hence timers_deinit();
	char* *argv;
} timer;

extern int timers_quitting;
extern unsigned long utime(void);
extern unsigned long wait_until_core(unsigned long desired_usec);
extern unsigned long wait_until(unsigned long desired_usec);
// Adds a new timer. If usec is 0, automatically clears the timers *when retrieved with timer_get*.
// The reason for this behavior is to simplify injecting a timer for an immediate switch.
// NOTE: It is assumed argv is freeable once unused.
extern int timer_add(unsigned long usec, int moduleno, int argc, char* argv[]);
extern timer timer_get(void);

// If argv != NULL: all args will be freed if relevant, then argv will be freed
extern void timer_free_argv(int argc, char ** argv);

// Regarding these, I'm drawing a distinction between "timer" as in an individual, and timers, the service,
//  to try and keep naming consistent with the X.h -> X_ scheme.

// Used to create the mutex. outmodno may be changed after (the module data is copied)
extern int timers_init(int outmodno);

// Tell timers to quit gracefully.
extern void timers_doquit(void);
// Used to ensure all argv's freed and destroy the mutex.
extern int timers_deinit(void);
