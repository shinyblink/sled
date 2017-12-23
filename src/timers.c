// Timers.
// Very basic, but enough for this.

#include <types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

typedef struct timer {
	int moduleno;
	ulong time; // time in microseconds.

	int argc;
	char* *argv;
} timer;

static struct timer TIMERS[MAX_TIMERS];
static int timer_count = 0;

static pthread_mutex_t tlock;

ulong utime(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		printf("Failed to get the time???\n");
		exit(1);
	}
	return T_SECOND * tv.tv_sec + tv.tv_usec;
}

ulong wait_until(ulong desired_usec) {
	ulong tnow = utime();
	if (tnow >= desired_usec)
		return tnow;
	useconds_t sleeptime = desired_usec - tnow;
	usleep(sleeptime);
	return desired_usec;
}

int timer_add(ulong usec,int moduleno, int argc, char* argv[]) {
	struct timer t = { .moduleno = moduleno, .time = usec, .argc = argc, .argv = argv };

	pthread_mutex_lock(&tlock);
	TIMERS[timer_count] = t;
	timer_count++;
	pthread_mutex_unlock(&tlock);
	return 0;
}

// Select the soonest timer, return it and clean up the spot it left.
timer timer_get(void) {
	timer t = { .moduleno = -1, .time = 0};
	if (timer_count == 0)
		return t;

	pthread_mutex_lock(&tlock);

	// Find the soonest/smallest timer.
	int i;
	int smallest = 0;
	ulong min = TIMERS[0].time;
	if (timer_count > 1)
		for (i = 1; i < timer_count; i++)
			if (min > TIMERS[i].time) {
				smallest = i;
				min = TIMERS[i].time;
			}

	// Keep it.
	t = TIMERS[smallest];

	// Move things back.
	memmove(&TIMERS[smallest], &TIMERS[smallest+1], timer_count - smallest - 1);
	timer_count--;

	pthread_mutex_unlock(&tlock);
	return t;
}
