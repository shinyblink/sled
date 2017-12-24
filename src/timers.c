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

int timers_quitting = 0;

static pthread_mutex_t tlock;

void timer_free_argv(int argc, char ** argv);

ulong utime(void) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		printf("Failed to get the time???\n");
		exit(1);
	}
	return T_SECOND * tv.tv_sec + tv.tv_usec;
}

// The critical wait_until code
inline ulong wait_until_core(ulong desired_usec) {
	ulong tnow = utime();
	if (tnow >= desired_usec)
		return tnow;
	useconds_t sleeptime = desired_usec - tnow;
	usleep(sleeptime);
	return desired_usec;
}

#ifndef PLATFORM_SDL2

ulong wait_until(ulong desired_usec) {
	return wait_until_core(desired_usec);
}

#else

#include <SDL2/SDL.h>

ulong wait_until(ulong desired_usec) {
	SDL_Event ev;
	while (1) {
		ulong tnow = utime();
		if (tnow >= desired_usec)
			return tnow;
		useconds_t sleeptimems = (desired_usec - tnow) / 1000;
		if (SDL_WaitEventTimeout(&ev, sleeptimems)) {
			if (ev.type == SDL_QUIT) {
				timers_quitting = 1;
				return utime();
			}
		} else {
			return wait_until_core(desired_usec);
		}
	}
}

#endif

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

	pthread_mutex_lock(&tlock);

	timer t = { .moduleno = -1, .time = 0};
	if (timer_count == 0) {
		pthread_mutex_unlock(&tlock);
		return t;
	}

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

	if (t.time == 0) {
		// Clear all timers safely. Note that this timer's argc/argv is being used.
		int i;
		for (i = 0; i < timer_count; i++)
			if (i != smallest)
				timer_free_argv(TIMERS[i].argc, TIMERS[i].argv);
		timer_count = 0;
	} else {
		// Move things back.
		memmove(&TIMERS[smallest], &TIMERS[smallest+1], (timer_count - smallest - 1) * sizeof(timer));
		timer_count--;
	}

	pthread_mutex_unlock(&tlock);
	return t;
}

void timer_free_argv(int argc, char ** argv) {
	if (argv) {
		int i;
		for (i = 0; i < argc; i++)
			free(argv[i]);
		free(argv);
	}
}

int timers_init(void) {
	if (pthread_mutex_init(&tlock, NULL))
		return 1;
	return 0;
}

int timers_deinit(void) {
	pthread_mutex_destroy(&tlock);
	int i;
	for (i = 0; i < timer_count; i++)
		timer_free_argv(TIMERS[i].argc, TIMERS[i].argv);
	return 0;
}
