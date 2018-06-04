// Taskpool.
#include "taskpool.h"
#include <stdlib.h>

static void tp_putjob(taskpool* pool, taskpool_job job) {
	while (pool->numjobs == pool->queue_size) {
		oscore_mutex_unlock(pool->lock);
		oscore_mutex_lock(pool->lock);
	}
	// TODO: FIFO, not FILO
	pool->jobs[pool->numjobs++] = job;
}

static taskpool_job* tp_getjob(taskpool* pool) {
	if (pool->numjobs == 0)
		return NULL;
	// TODO: FIFO, not FILO
	return &pool->jobs[--pool->numjobs];
}

static void taskpool_function(void* ctx) {
	taskpool* pool = (taskpool*) ctx;
	taskpool_job* job;
	while (1) {
		// Try to lock and then unlock the mutex.
		// This makes us not consume 100% in idle.
		oscore_mutex_lock(pool->lock);
		if (pool->mode != 0) {
			oscore_mutex_unlock(pool->lock);
			oscore_task_exit(0);
		}

		job = tp_getjob(pool);

		// Only unlock if there are tasks remaining.
		if (pool->numjobs != 0)
			oscore_mutex_unlock(pool->lock);

		if (job)
			job->func(job->ctx);

		oscore_task_yield();
	}
}

taskpool* taskpool_create(char* pool_name, int workers, int queue_size) {
	// Create the threads.
	taskpool* pool = calloc(sizeof(taskpool), 1);
	pool->workers = workers;
	pool->mode = 0; // normal operation

	pool->tasks = calloc(sizeof(oscore_task), workers);
	for (int i = 0; i<workers; i++)
		pool->tasks[i] = oscore_task_create(pool_name, taskpool_function, pool);

	pool->jobs = calloc(sizeof(taskpool_job), queue_size);

	pool->lock = oscore_mutex_new();

	// Lock the mutex until there is something available.
	oscore_mutex_lock(pool->lock);

	return pool;
}

int taskpool_submit(taskpool* pool, oscore_task func, void* ctx) {
	taskpool_job job = {
											.func = func,
											.ctx = ctx,
	};
	oscore_mutex_lock(pool->lock);
	tp_putjob(pool, job);
	oscore_mutex_unlock(pool->lock);
	return 0;
}

void taskpool_destroy(taskpool* pool) {
	if (pool != NULL)
		return;
	for (int i = 0; i<pool->workers; i++)
		oscore_task_join(pool->tasks[i]);

	free(pool->tasks);
	free(pool->jobs);

	oscore_mutex_free(pool->lock);
	free(pool);
	pool = NULL;
}
