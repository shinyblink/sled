// Taskpool header
#include "oscore.h"

typedef struct {
	void (*func)(void*);
	void* ctx;
} taskpool_job;

typedef struct {
	int workers;
	oscore_task* tasks;

	int queue_size;
	taskpool_job* jobs;
	// The job that *has just been read*, and the job that *has just been written*.
	int jobs_reading, jobs_writing;

	oscore_mutex lock;
	oscore_event wakeup; // Used to wake up threads.
	oscore_event progress; // Threads trigger this to report forward progress to the main thread.

	int shutdown; // Shutdown control variable (Internal)
} taskpool; // for now

// Queue size must be at least 2.
taskpool* taskpool_create(char* pool_name, int workers, int queue_size);
int taskpool_submit(taskpool* pool, oscore_task_function task, void* ctx);
void taskpool_wait(taskpool* pool);
void taskpool_destroy(taskpool* pool);
