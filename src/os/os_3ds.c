// os_3ds: For running sled on the Nintendo 3ds, under it's microkernel OS.
// Still needs some filling in.
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

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include <3ds.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define TOHANDLE(ev) (* (Handle*) (ev))

// Main entry point.
int main(int argc, char** argv) {
	// Enable better performance on n3ds.
	osSetSpeedupEnable(true);
	return sled_main(argc, argv);
}

// -- event
oscore_event oscore_event_new(void) {
	Handle* event = calloc(1, sizeof(Handle));
	svcCreateEvent(event, RESET_STICKY);
	return event;
}

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	oscore_time tnow = udate();
	if (tnow >= desired_usec)
		return tnow;
	oscore_time sleeptime = desired_usec - tnow;

	Result res = svcWaitSynchronization(TOHANDLE(ev), sleeptime * 1000);
	if (R_FAILED(res))
		return 0; // timeout

	return 1; // signal
}

void oscore_event_signal(oscore_event ev) {
	svcSignalEvent(TOHANDLE(ev));
}

void oscore_event_free(oscore_event ev) {
	svcCloseHandle(TOHANDLE(ev));
	free(ev);
}

// Time keeping.
oscore_time oscore_udate(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		printf("Failed to get the time???\n");
		exit(1);
	}
	return T_SECOND * tv.tv_sec + tv.tv_usec;
}

// Threading
oscore_task oscore_thread_create(const char* name, oscore_task_function func, void* ctx) {
	// uuh
	return NULL;
}

void oscore_task_yield(void) {
	// nothing.
};

void oscore_task_exit(void * status) {
	// nope
};

void * oscore_task_join(oscore_task task) {
	// ye ok
	return 0;
};

int oscore_ncpus(void) {
	return 1; // For now.
}

void oscore_task_pin(oscore_task task, int cpu) {}

// -- mutex
// TODO: fill this in.
oscore_mutex oscore_mutex_new(void) {
	return NULL;
}

void oscore_mutex_lock(oscore_mutex m) {
}

void oscore_mutex_unlock(oscore_mutex m) {
}

void oscore_mutex_free(oscore_mutex m) {
}
