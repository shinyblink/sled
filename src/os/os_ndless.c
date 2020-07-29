// os_ndless: clone of os_dummy for now
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

#include <libndls.h>

// Main entry point.
int main(int argc, char** argv) {
	return sled_main(argc, argv);
}

// h a c k because ndless only has msleep
int usleep(useconds_t __useconds) {
    return msleep(__useconds / 1000);
}

// -- event
oscore_event oscore_event_new(void) {
	return NULL;
}

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	oscore_time tnow = udate();
	if (tnow >= desired_usec)
		return tnow;
	oscore_time sleeptime = desired_usec - tnow;

	usleep(sleeptime);
	return 0;

}

void oscore_event_signal(oscore_event ev) {
}

void oscore_event_free(oscore_event ev) {
}

// Time keeping.
// Note, this should be replaced.
oscore_time oscore_udate(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		printf("Failed to get the time???\n");
		exit(1);
	}
	return T_SECOND * tv.tv_sec + tv.tv_usec;
}

// Threading
oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx) {
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
	return 1;
}

void oscore_task_pin(oscore_task task, int cpu) {}

void oscore_task_setprio(oscore_task task, int prio) {}

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
