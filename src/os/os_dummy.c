// os_dummy: for non-multithreaded systems only

#include <types.h>
#include <oscore.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

// -- event

oscore_event oscore_event_new(void) {
	return NULL;
}

int oscore_event_wait(oscore_event ev, ulong sleeptime) {
	usleep(sleeptime);
	return 1;
}

void oscore_event_signal(oscore_event ev) {
}

void oscore_event_free(oscore_event ev) {
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
