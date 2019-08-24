// Timers.
// Very basic, but enough for this.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mod.h"
#include "asl.h"
#include "oscore.h"
#include "main.h"
#include "timers.h"

static struct timer TIMERS[MAX_TIMERS];
static int timer_count = 0;

int timers_quitting = 0;

static oscore_mutex tlock;

static oscore_event breakpipe;

// udate has been replaced by oscore.
// No, this is not pretty.
oscore_time udate(void) {
	return oscore_udate();
}

// The critical wait_until code
oscore_time timers_wait_until_core(oscore_time desired_usec) {
	if (oscore_event_wait_until(breakpipe, desired_usec))
		return udate();
	return desired_usec;
}

void timers_wait_until_break_cleanup_core(void) {
	oscore_event_wait_until(breakpipe, 0);
}

void timers_wait_until_break_core(void) {
	oscore_event_signal(breakpipe);
}

// This code calls into the output module's wait_until impl.
static module *out;
static int outmodno;
oscore_time timers_wait_until(oscore_time desired_usec) {
	return out->wait_until(outmodno, desired_usec);
}

// This code calls into the output module's wait_until_break impl.
void timers_wait_until_break(void) {
	return out->wait_until_break(outmodno);
}

int timer_add(oscore_time usec,int moduleno, int argc, char* argv[]) {
	struct timer t = { .moduleno = moduleno, .time = usec, .args = {argc, argv}};

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
	oscore_time min = TIMERS[0].time;
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
				asl_clearav(&TIMERS[i].args);
		timer_count = 0;
	} else {
		// Move things back.
		memmove(&TIMERS[smallest], &TIMERS[smallest+1], (timer_count - smallest - 1) * sizeof(timer));
		timer_count--;
	}

	oscore_mutex_unlock(tlock);
	return t;
}

int timers_init(int omno) {
	outmodno = omno;
	tlock = oscore_mutex_new();
	breakpipe = oscore_event_new();
	out = mod_get(outmodno);
	return 0;
}

void timers_doquit(void) {
	timers_quitting = 1;
	timers_wait_until_break();
}

int timers_deinit(void) {
	oscore_mutex_free(tlock);
	oscore_event_free(breakpipe);
	int i;
	for (i = 0; i < timer_count; i++)
		asl_clearav(&TIMERS[i].args);
	return 0;
}
