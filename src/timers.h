// Timers

#ifndef __INCLUDED_TIMERS__
#define __INCLUDED_TIMERS__

#include "asl.h"
#include "types.h"

typedef struct timer {
	// Special values for this:
	// -1: No timer available
	// -2: No *module* available
	int moduleno;
	oscore_time time;

	// Malloc'd data containing strdup/malloc'd data, hence timers_deinit();
	asl_av_t args;
} timer;

extern int timers_quitting;
extern oscore_time udate(void);

// Generic shared implementation among non-eventloop stuff
extern oscore_time timers_wait_until_core(oscore_time desired_usec);
// Used by output modules that sometimes don't and sometimes do use wait_until_core,
//  and thus might not properly clean out the pipe otherwise.
// Should only be used when acknowledging a break.
extern void timers_wait_until_break_cleanup_core(void);
extern void timers_wait_until_break_core(void);

extern oscore_time timers_wait_until(oscore_time desired_usec);
extern void timers_wait_until_break(void);

// Adds a new timer. If usec is 0, automatically clears the timers *when retrieved with timer_get*.
// The reason for this behavior is to simplify injecting a timer for an immediate switch.
// NOTE: It is assumed argv is freeable once unused.
extern int timer_add(oscore_time usec, int moduleno, int argc, char* argv[]);
extern timer timer_get(void);


// Regarding these, I'm drawing a distinction between "timer" as in an individual, and timers, the service,
//  to try and keep naming consistent with the X.h -> X_ scheme.

// Used to create the mutex. outmodno may be changed after (the module data is copied)
extern int timers_init(int outmodno);

// Tell timers to quit gracefully.
extern void timers_doquit(void);
// Used to ensure all argv's freed and destroy the mutex.
extern int timers_deinit(void);

#endif
