// os_unix
// The platform specific code for UNIX-likes.

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

// Main method.
int main(int argc, char** argv) {
	return sled_main(argc, argv);
}

// -- event
typedef struct {
	int send;
	int recv;
} oscore_event_i;

int oscore_event_wait(oscore_event ev, ulong sleeptime) {
	oscore_event_i * oei = (oscore_event_i *) ev;
	struct timeval timeout;
	timeout.tv_sec = sleeptime / 1000000;
	timeout.tv_usec = sleeptime % 1000000;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(oei->recv, &set);
	if (select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
		char buf[512];
		read(oei->recv, buf, 512);
		return 0;
	}
	return 1;
}

void oscore_event_signal(oscore_event ev) {
	oscore_event_i * oei = (oscore_event_i *) ev;
	char discard = 0;
	write(oei->send, &discard, 1);
}

oscore_event oscore_event_new(void) {
	oscore_event_i * oei = malloc(sizeof(oscore_event_i));
	int fd[2];
	assert(oei);
	pipe(fd);
	oei->recv = fd[0];
	oei->send = fd[1];
	return oei;
}

void oscore_event_free(oscore_event ev) {
	oscore_event_i * oei = (oscore_event_i *) ev;
	close(oei->send);
	close(oei->recv);
	free(oei);
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
