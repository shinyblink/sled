// Taskpool header
#ifndef __INCLUDED_TASKPOOL__
#define __INCLUDED_TASKPOOL__

#include "types.h"
#include "oscore.h"
#include <stddef.h>

#ifdef __linux__
// need to check where this works as well.
// it's using pipe() and some fnctls.
#define TP_PIPES
#endif

#ifdef TP_PIPES
#define TP_CMD_JOB 1
#define TP_CMD_DONE 2
#define TP_CMD_QUIT 3
typedef struct {
		byte type;
		int pos;
} taskpool_command;
#endif

typedef struct {
	void (*func)(void*);
	void* ctx;
} taskpool_job;

typedef struct {
	int workers;
	oscore_task* tasks;

	int queue_size;
	taskpool_job* jobs;

#ifdef TP_PIPES
	int cmdpipe[2];
	int retpipe[2];

#else
	// The job that *has just been read*, and the job that *has just been written*.
	int jobs_reading, jobs_writing;

	oscore_mutex lock;
	oscore_event wakeup; // Used to wake up threads.
	oscore_event progress; // Threads trigger this to report forward progress to the main thread.

	int shutdown; // Shutdown control variable (Internal)
#endif
} taskpool; // for now

// Queue size must be at least 2.
taskpool* taskpool_create(const char* pool_name, int workers, int queue_size);
int taskpool_submit(taskpool* pool, void (*task)(void*), void* ctx);

void taskpool_wait(taskpool* pool);
void taskpool_destroy(taskpool* pool);

taskpool* TP_GLOBAL __attribute__((weak));


// Hellish stuff to run stuff in parallel simpler.
void taskpool_submit_array(taskpool* pool, int count, void (*func)(void*), void* ctx, size_t size );
void taskpool_forloop(taskpool* pool, void (*func)(void*), int start, int end);
void taskpool_forloop_free(void);

#endif
