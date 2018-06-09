// os_3ds: For running sled on the Nintendo 3ds, under it's microkernel OS.
// Still needs some filling in.

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

int oscore_event_wait_until(oscore_event ev, ulong desired_usec) {
	ulong tnow = udate();
	if (tnow >= desired_usec)
		return tnow;
	ulong sleeptime = desired_usec - tnow;

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
ulong oscore_udate(void) {
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

void oscore_task_exit(int status) {
	// nope
};

void * oscore_task_join(oscore_task task) {
	// ye ok
	return 0;
};

int oscore_ncpus(void) {
	return 1; // For now.
}

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
