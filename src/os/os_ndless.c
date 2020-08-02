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
// needed for the IO macro
#include <os.h>

// Main entry point.
int main(int argc, char** argv) {
	return sled_main(argc, argv);
}

// h a c k because ndless only has msleep
// so, we ripped off msleep with a different load value.
// didn't make much of a difference though.
int nanosleep(useconds_t __useconds) {
    // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0271d/CHDFDDCF.html
    volatile unsigned *load = (unsigned*)0x900D0000;
    volatile unsigned *control = (unsigned*)0x900D0008;
    volatile unsigned *int_clear = (unsigned*)0x900D000C;
    volatile unsigned *int_status = (unsigned*)0x900D0010;
    unsigned orig_control = *control;
    unsigned orig_load = *load;
    *control = 0; // disable timer
    *int_clear = 1; // clear interrupt status
    *load = __useconds / 32;
    *control = 0b01100011; // disabled, TimerMode N/A, int, no prescale, 32-bit, One Shot -> 32khz
    *control = 0b11100011; // enable timer

    // Can't use idle() here as it acks the timer interrupt
    volatile unsigned *intmask = IO(0xDC000008, 0xDC000010);
    unsigned orig_mask = intmask[0];
    intmask[1] = ~(1 << 19); // Disable all IRQs except timer

    while ((*int_status & 1) == 0)
            __asm volatile("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) ); // Wait for an interrupt to occur

    intmask[1] = 0xFFFFFFFF; // Disable all IRQs
    intmask[0] = orig_mask; // renable IRQs

    *control = 0; // disable timer
    *int_clear = 1; // clear interrupt status
    *control = orig_control & 0b01111111; // timer still disabled
    *load = orig_load;
    *control = orig_control; // enable timer

    return 0;
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

	nanosleep(sleeptime);
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
