// os_3ds: For running sled on the Nintendo 3ds, under it's microkernel OS.
// Still needs some filling in.

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include <3ds.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

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

int oscore_event_wait(oscore_event ev, ulong sleeptime) {
	Result res = svcWaitSynchronization(TOHANDLE(ev), sleeptime);
	if (R_FAILED(res))
		return 1;

	return 0;
}

void oscore_event_signal(oscore_event ev) {
	svcSignalEvent(TOHANDLE(ev));
}

void oscore_event_free(oscore_event ev) {
	free(ev);
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
