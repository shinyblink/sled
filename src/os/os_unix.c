// os_unix
// The platform specific code for UNIX-likes.
// Mostly POSIX, but might contain some extensions.
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

#define _GNU_SOURCE

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <pthread_np.h>
#endif

// Main method.
int main(int argc, char** argv) {
	return sled_main(argc, argv);
}

// -- event
typedef struct {
	int send;
	int recv;
} oscore_event_i;

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	oscore_time tnow = udate();
	if (tnow >= desired_usec)
		return tnow;
	oscore_time sleeptime = desired_usec - tnow;

	oscore_event_i * oei = (oscore_event_i *) ev;
	struct timeval timeout;
	timeout.tv_sec = sleeptime / 1000000;
	timeout.tv_usec = sleeptime % 1000000;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(oei->recv, &set);
	if (select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
		char buf[512];
		// Non-blocking (see creation). It DOESN'T MATTER if this errors!!!
		read(oei->recv, buf, 512);
		return 1; // we got an interrupt
	}
	return 0; // timeout
}

void oscore_event_signal(oscore_event ev) {
	oscore_event_i * oei = (oscore_event_i *) ev;
	char discard = 0;
	ssize_t ret = write(oei->send, &discard, 1);
	if(ret < 0) {
		perror("oscor_event_signal write error");
		exit(1);
	}
}

oscore_event oscore_event_new(void) {
	oscore_event_i * oei = malloc(sizeof(oscore_event_i));
	int fd[2];
	assert(oei);
	int ret = pipe(fd);
	if(ret < 0) {
		perror("oscor_event_new pipe error");
		exit(1);
	}
	oei->recv = fd[0];
	// Make the receiver non-blocking to avoid accidentally causing lockups in wait_until
	fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
	oei->send = fd[1];
	return oei;
}

void oscore_event_free(oscore_event ev) {
	oscore_event_i * oei = (oscore_event_i *) ev;
	close(oei->send);
	close(oei->recv);
	free(oei);
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
oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx) {
	pthread_t* thread = calloc(1, sizeof(pthread_t));
	pthread_create(thread, NULL, (void*) func, ctx);

#if defined(__linux__) || defined(__NetBSD__)
	pthread_setname_np(*thread, name);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(*thread, name);
#endif
	return thread;
}

void oscore_task_yield(void) {
#ifdef pthread_yield
	pthread_yield();
#else
#include <sched.h>
	sched_yield();
#endif
}

void oscore_task_exit(void * status) {
	pthread_exit(status);
}

void * oscore_task_join(oscore_task task) {
	void * retval = 0;
	if (pthread_join(*((pthread_t *) task), (void*) &retval)) {
		free(task);
		return NULL;
	}
	free(task);
	return retval;
}

void oscore_task_setprio(oscore_task task, int prio) {
struct sched_param param;
	int policy;
	pthread_getschedparam(pthread_self(), &policy, &param);

#if defined(__linux__)
	if (prio == TPRIO_LOW)
		policy = SCHED_BATCH;
#endif
	param.sched_priority++; // decrease priority.

	pthread_setschedparam(*(pthread_t*) task, policy, &param);
}

int oscore_ncpus(void) {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

void oscore_task_pin(oscore_task task, int cpu) {
#ifdef pthread_setaffinity_np
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	pthread_setaffinity_np(*((pthread_t*) task), sizeof(cpu_set_t), &cpuset);
#endif
}

// -- mutex
oscore_mutex oscore_mutex_new(void) {
	pthread_mutex_t * mutex = malloc(sizeof(pthread_mutex_t));
	assert(mutex);
	assert(!pthread_mutex_init(mutex, NULL));
	return mutex;
}

void oscore_mutex_lock(oscore_mutex m) {
	pthread_mutex_t * mutex = m;
	pthread_mutex_lock(mutex);
}

void oscore_mutex_unlock(oscore_mutex m) {
	pthread_mutex_t * mutex = m;
	pthread_mutex_unlock(mutex);
}

void oscore_mutex_free(oscore_mutex m) {
	pthread_mutex_t * mutex = m;
	pthread_mutex_destroy(mutex);
	free(mutex);
}
