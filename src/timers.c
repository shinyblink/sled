// Timers.
// Very basic, but enough for this.

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mod.h"
#include "asl.h"
#include "oscore.h"
#include "main.h"

typedef struct timer {
	int moduleno;
	ulong time; // time in microseconds.

	int argc;
	char* *argv;
} timer;

static struct timer TIMERS[MAX_TIMERS];
static int timer_count = 0;

int timers_quitting = 0;

static oscore_mutex tlock;

static oscore_event breakpipe;

// udate has been replaced by oscore.
// No, this is not pretty.
ulong udate(void) {
	return oscore_udate();
}

// The critical wait_until code
ulong wait_until_core(ulong desired_usec) {
	if (oscore_event_wait_until(breakpipe, desired_usec))
		return udate();
	return desired_usec;
}

void wait_until_break_cleanup_core(void) {
	oscore_event_wait_until(breakpipe, 0);
}

void wait_until_break_core(void) {
	oscore_event_signal(breakpipe);
}

// This code calls into the output module's wait_until impl.
mod_out *out;
ulong wait_until(ulong desired_usec) {
	return out->wait_until(desired_usec);
}

// This code calls into the output module's wait_until_break impl.
void wait_until_break(void) {
	return out->wait_until_break();
}

int timer_add(ulong usec,int moduleno, int argc, char* argv[]) {
	struct timer t = { .moduleno = moduleno, .time = usec, .argc = argc, .argv = argv };

	oscore_mutex_lock(tlock);
	if (timer_count >= MAX_TIMERS) {
		oscore_mutex_unlock(tlock);
		return 1;
	}
	TIMERS[timer_count] = t;
	timer_count++;
	oscore_mutex_unlock(tlock);
	return 0;
}

// Select the soonest timer, return it and clean up the spot it left.
timer timer_get(void) {
	oscore_mutex_lock(tlock);

	timer t = { .moduleno = -1, .time = 0};
	if (timer_count == 0) {
		oscore_mutex_unlock(tlock);
		return t;
	}

	// Find the soonest/smallest timer.
	int smallest = 0;
	ulong min = TIMERS[0].time;
	if (timer_count > 1)
		for (int i = 1; i < timer_count; i++)
			if (min > TIMERS[i].time) {
				smallest = i;
				min = TIMERS[i].time;
			}

	// Keep it.
	t = TIMERS[smallest];

	if (t.time == 0) {
		// Clear all timers safely. Note that this timer's argc/argv is being used.
		for (int i = 0; i < timer_count; i++)
			if (i != smallest)
				asl_free_argv(TIMERS[i].argc, TIMERS[i].argv);
		timer_count = 0;
	} else {
		// Move things back.
		memmove(&TIMERS[smallest], &TIMERS[smallest+1], (timer_count - smallest - 1) * sizeof(timer));
		timer_count--;
	}

	oscore_mutex_unlock(tlock);
	return t;
}

int timers_init(int outmodno) {
	tlock = oscore_mutex_new();
	breakpipe = oscore_event_new();
	out = mod_get(outmodno)->mod;
	return 0;
}

void timers_doquit(void) {
	timers_quitting = 1;
	wait_until_break();
}

int timers_deinit(void) {
	oscore_mutex_free(tlock);
	oscore_event_free(breakpipe);
	int i;
	for (i = 0; i < timer_count; i++)
		asl_free_argv(TIMERS[i].argc, TIMERS[i].argv);
	return 0;
}
