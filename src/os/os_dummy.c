// os_dummy: for non-multithreaded systems only
// Lots of stubs.

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// Main entry point.
int main(int argc, char** argv) {
	return sled_main(argc, argv);
}

// -- event
oscore_event oscore_event_new(void) {
	return NULL;
}

int oscore_event_wait_until(oscore_event ev, ulong desired_usec) {
	ulong tnow = udate();
	if (tnow >= desired_usec)
		return tnow;
	ulong sleeptime = desired_usec - tnow;

	usleep(sleeptime);
	return 1;
}

void oscore_event_signal(oscore_event ev) {
}

void oscore_event_free(oscore_event ev) {
}

// Time keeping.
// Note, this should be replaced.
ulong oscore_udate(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		printf("Failed to get the time???\n");
		exit(1);
	}
	return T_SECOND * tv.tv_sec + tv.tv_usec;
}

// -- mutex
oscore_mutex oscore_mutex_new(void) {
	return NULL;
}

void oscore_mutex_lock(oscore_mutex m) {
}

void oscore_mutex_unlock(oscore_mutex m) {
}

void oscore_mutex_free(oscore_mutex m) {
}
