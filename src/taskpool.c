// Taskpool.
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

#include "taskpool.h"
#include <stdlib.h>
#include "types.h"
#include "timers.h"
#include <stdlib.h>

// So, the locking system needed a bit of a rethink...
// (the taskpool can't run until the queue is full)
// In particular, it didn't consider the existence of oscore_event,
//  which exists for the specific purpose of not-breaking-things...

// The FIFO is a ring.
// There are two indexes (the "reading pointer" and "writing pointer"),
//  both of which represent the last processed index.
// The writing pointer refuses to reach the reading pointer,
//  which would turn a full ring into an empty one,
// and the reading pointer refuses to go *past* the writing pointer,
//  which would turn an empty ring into a full one.

static inline void tp_putjob(taskpool* pool, taskpool_job job) {
	oscore_mutex_lock(pool->lock);
	// Cleanup the event coming in
	oscore_event_wait_until(pool->progress, 0);
	while (1) {
		// If we're going to run into the reading pointer from behind...
		int nx = (pool->jobs_writing + 1) % pool->queue_size;
		if (nx != pool->jobs_reading) {
			pool->jobs_writing = nx;
			break;
		}
		// Yup, we are, so wait until progress is made so we don't
		oscore_mutex_unlock(pool->lock);
		// Wait for 5ms as a fallback in case something goes wrong w/ progress.
		// Should only ever be a problem with multiple writers, but...
		oscore_event_wait_until(pool->progress, udate() + 5000UL);
		oscore_mutex_lock(pool->lock);
	}
	// Update the job, and unlock
	pool->jobs[pool->jobs_writing] = job;
	oscore_mutex_unlock(pool->lock);
	oscore_event_signal(pool->wakeup);
}

static inline taskpool_job tp_getjob(taskpool* pool) {
	taskpool_job job = {
		.func = NULL,
		.ctx = NULL
	};
	oscore_mutex_lock(pool->lock);
	// If we aren't already at the end of all that's written...
	if (pool->jobs_reading != pool->jobs_writing) {
		pool->jobs_reading = (pool->jobs_reading + 1) % pool->queue_size;
		job = pool->jobs[pool->jobs_reading];
		oscore_mutex_unlock(pool->lock); // Note: This should be before the signal in case that has a scheduling effect.
		// Made progress.
		oscore_event_signal(pool->progress);
		return job;
	}
	oscore_mutex_unlock(pool->lock);
	return job;
}

static void * taskpool_function(void* ctx) {
	taskpool* pool = (taskpool*) ctx;
	taskpool_job job;
	while (1) {
		// Wait for the event to be triggered that says a task is ready. Wait 50ms, since any issues should only occur on shutdown.
		oscore_event_wait_until(pool->wakeup, udate() + 50000UL);
		while (1) {
			// It's possible that the event we just got covered multiple tasks.
			// Accept as many tasks as possible during our active time.
			if (pool->shutdown) {
				// Should help speed up shutdown.
				oscore_event_signal(pool->wakeup);
				return NULL;
			}

			job = tp_getjob(pool);

			if (job.func) {
				job.func(job.ctx);
				// We did a job. Now yield for RT sanity
				oscore_task_yield();
			} else {
				break;
			}
		}
	}
}

taskpool* taskpool_create(const char* pool_name, int workers, int queue_size) {
	// Create the threads.
	taskpool* pool = calloc(sizeof(taskpool), 1);
	pool->queue_size = queue_size;
	// jobs_reading/jobs_writing at correct positions on startup

	pool->tasks = calloc(sizeof(oscore_task), (size_t) workers);

	pool->jobs = calloc(sizeof(taskpool_job), (size_t) queue_size);

	pool->lock = oscore_mutex_new();
	pool->wakeup = oscore_event_new();
	pool->progress = oscore_event_new();

	// -- Do this last. It's thread creation. --

	int no_cpus = oscore_ncpus();

	// If the oscore has NO thread support, not even faking, we still want basic GFX modules that use taskpool to not break.
	// So if pool->workers is 0, then we're just pretending.
	pool->workers = 0;
	if (workers > 1) {
		for (int i = 0; i < workers; i++) {
			pool->tasks[pool->workers] = oscore_task_create(pool_name, taskpool_function, pool);
			if (pool->tasks[pool->workers]) {
				oscore_task_setprio(pool->tasks[pool->workers], TPRIO_LOW);
				if (no_cpus == workers)
					oscore_task_pin(pool->tasks[pool->workers], pool->workers);
				pool->workers++;
			}
		}
		pool->workers = workers;
	} else {
		pool->workers = 1;
	}

	return pool;
}

int taskpool_submit(taskpool* pool, void (*func)(void*), void* ctx) {
	if (pool->workers <= 1) {
		// We're faking. This isn't a real taskpool.
		func(ctx);
		return 0;
	}
	taskpool_job job = {
		.func = func,
		.ctx = ctx,
	};
	tp_putjob(pool, job);
	return 0;
}

// Hellish stuff to run stuff in parallel.
inline void taskpool_submit_array(taskpool* pool, int count, void (*func)(void*), void* ctx, size_t size) {
	for (int i = 0; i < count; i++)
		taskpool_submit(pool, func, ctx + (i * size));
}


// Since we
static int* taskpool_numbers;
static int taskpool_numbers_maxn = 0;

void taskpool_forloop(taskpool* pool, void (*func)(void*), int start, int end) {
	int s = MAX(start, 0);
	int c = (end) - s;

	if (end > taskpool_numbers_maxn) {
		taskpool_numbers = realloc(taskpool_numbers, end * sizeof(int));;
		assert(taskpool_numbers);

		for (int i = taskpool_numbers_maxn; i < end; i++)
			taskpool_numbers[i] = i;

		taskpool_numbers_maxn = end;
	}

	taskpool_submit_array(pool, c, func, &taskpool_numbers[s], sizeof(int));
}

void taskpool_forloop_free(void) {
	if (taskpool_numbers)
		free(taskpool_numbers);
}

void taskpool_wait(taskpool* pool) {
	// We're waiting for tasks to finish.
	oscore_mutex_lock(pool->lock);
	// While the task list isn't empty, unlock, wait for progress, then lock again.
	while (pool->jobs_reading != pool->jobs_writing) {
		oscore_mutex_unlock(pool->lock);
		oscore_event_wait_until(pool->progress, udate() + 50000UL);
		oscore_mutex_lock(pool->lock);
	}
	// Clear any remaining progress events.
	oscore_event_wait_until(pool->progress, 0);
	oscore_mutex_unlock(pool->lock);
}

void taskpool_destroy(taskpool* pool) {
	if (pool == NULL)
		return;
	pool->shutdown = 1;
	// This relies on the timeout for now in case wakeup only reaches one of the threads.
	// We don't know *which* thread gets it, so we don't know what to join.
	// (NOTE: If we're faking, this simply does nothing, as workers == 0)
	oscore_event_signal(pool->wakeup);
	if (pool->workers > 1)
		for (int i = 0; i < pool->workers; i++)
			oscore_task_join(pool->tasks[i]);

	free(pool->tasks);
	free(pool->jobs);

	oscore_mutex_free(pool->lock);
	oscore_event_free(pool->progress);
	oscore_event_free(pool->wakeup);
	free(pool);
}
