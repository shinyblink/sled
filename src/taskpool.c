// Taskpool.
#define _GNU_SOURCE
#include "taskpool.h"
#include <stdlib.h>
#include "types.h"
#include "timers.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include <stdatomic.h>

static void * taskpool_function(void* ctx) {
	taskpool* pool = (taskpool*) ctx;
	taskpool_queue_object * qobj = atomic_load(&(pool->whead));
	taskpool_job job;
	taskpool_queue_object * qobjnxt;
	while (qobj) {
		byte objtype = qobj->type;
		// Shutdown ignores the wait-for-next
		if (objtype != TASKPOOL_QUEUE_SHUTDOWN) {
			while (!atomic_load_explicit(&(qobj->next), memory_order_acquire)) {
				// Burn CPU
				oscore_task_yield();
				oscore_event_wait_until(pool->incoming, oscore_udate() + 1000);
			}
		}
		qobjnxt = qobj->next;
		if (objtype == TASKPOOL_QUEUE_JOB)
			job = qobj->jobdata;
		// This may doom the object to being deleted, so don't read from here on in
		int rc = atomic_fetch_sub(&(qobj->refcount), 1);
		if (rc == pool->workers) {
			if (objtype == TASKPOOL_QUEUE_JOB) {
				// We've been elected for job execution
				if (job.func) {
					job.func(job.ctx);
					// We did a job. Now yield for RT sanity
					oscore_task_yield();
				}
			}
		} else if (rc == 1) {
			if (objtype == TASKPOOL_QUEUE_WAIT) {
				// All threads have passed checkpoint, signal
				oscore_event_signal(pool->waitover);
			}
			atomic_fetch_sub(&(pool->usage), 1);
			// We own qobj now, which means we can do this:
			qobj->next = 0;
			taskpool_queue_object * t2 = atomic_exchange_explicit(&(pool->fhead), qobj, memory_order_acq_rel);
			atomic_store_explicit(&(t2->next), qobj, memory_order_release);
		}
		// Shutdown has a next of 0
		qobj = qobjnxt;
	}
	return 0;
}

// Checked at submission
static int taskpool_overloaded(taskpool * pool) {
	int usage = atomic_load(&(pool->usage));
	return usage >= TASKPOOL_MAX_USAGE;
}

static taskpool_queue_object * taskpool_alloc_obj(taskpool * pool) {
	// This can't be done sanely without race conditions, but we don't want a lock, at least of the kind that'll introduce delays.
	// This only locks it against other writer threads (if any), and if it can't immediately lock it'll just malloc.
	taskpool_queue_object * locked = atomic_exchange_explicit(&(pool->ffoot), 0, memory_order_acq_rel);
	if (locked) {
		taskpool_queue_object * lx = atomic_load_explicit(&(locked->next), memory_order_acquire);
		if (lx) {
			// Advance, this 'unlocks'
			atomic_store(&(pool->ffoot), lx);
			return lx;
		}
		// Failed to advance, so restore things (unlock)
		atomic_store(&(pool->ffoot), locked);
	}
	taskpool_queue_object * tqo = malloc(sizeof(taskpool_queue_object));
	assert(tqo);
	return tqo;
}

static void taskpool_inject_obj(taskpool * pool, taskpool_queue_object * obj) {
	atomic_fetch_add(&(pool->usage), 1);
	// Now for the writing side of things.
	taskpool_queue_object * t2 = atomic_exchange_explicit(&(pool->whead), obj, memory_order_acq_rel);
	atomic_store_explicit(&(t2->next), obj, memory_order_release);
}

static void taskpool_new_job(taskpool * pool, taskpool_job job) {
	taskpool_queue_object * tqo = taskpool_alloc_obj(pool);
	tqo->type = TASKPOOL_QUEUE_JOB;
	tqo->refcount = pool->workers;
	tqo->next = 0;
	tqo->jobdata = job;
	taskpool_inject_obj(pool, tqo);
}
static void taskpool_new_cmd(taskpool * pool, byte cmd) {
	taskpool_queue_object * tqo = taskpool_alloc_obj(pool);
	tqo->type = cmd;
	tqo->refcount = pool->workers;
	tqo->next = 0;
	taskpool_inject_obj(pool, tqo);
}

taskpool* taskpool_create(const char* pool_name, int workers, int queue_size) {
	// Create the threads.
	taskpool* pool = calloc(sizeof(taskpool), 1);
	assert(pool);

	pool->waitover = oscore_event_new();
	pool->incoming = oscore_event_new();

	pool->tasks = calloc(sizeof(oscore_task), workers);
	assert(pool->tasks);

	taskpool_queue_object * tqo = malloc(sizeof(taskpool_queue_object));
	assert(tqo);
	tqo->type = TASKPOOL_QUEUE_NOP;
	tqo->refcount = pool->workers;
	tqo->next = 0;
	pool->whead = tqo;

	tqo = malloc(sizeof(taskpool_queue_object));
	// Only field that needs to be valid
	tqo->next = 0;
	assert(tqo);
	pool->fhead = tqo;
	pool->ffoot = tqo;

	// -- Do this last. It's thread creation. --

	int cpus_are_workers = oscore_ncpus() == workers;

	// If the oscore has NO thread support, not even faking, we still want basic GFX modules that use taskpool to not break.
	// So if pool->workers is 0, then we're just pretending.
	pool->workers = 0;
	for (int i = 0; i < workers; i++) {
		pool->tasks[pool->workers] = oscore_task_create(pool_name, taskpool_function, pool);
		if (pool->tasks[pool->workers]) {
			oscore_task_setprio(pool->tasks[pool->workers], TPRIO_LOW);
			if (cpus_are_workers)
				oscore_task_pin(pool->tasks[pool->workers], pool->workers);
			pool->workers++;
		}
	}

	return pool;
}

int taskpool_submit(taskpool* pool, void (*func)(void*), void* ctx) {
	if (taskpool_overloaded(pool) || (pool->workers == 0)) {
		// Faking due to unreal taskpool or overload condition.
		func(ctx);
		return 0;
	}
	taskpool_job job = {
		.func = func,
		.ctx = ctx,
	};
	taskpool_new_job(pool, job);
	taskpool_new_cmd(pool, TASKPOOL_QUEUE_NOP);
	return 0;
}

// Hellish stuff to run stuff in parallel.
inline void taskpool_submit_array(taskpool* pool, int count, void (*func)(void*), void* ctx, size_t size) {
	if (taskpool_overloaded(pool) || (pool->workers == 0)) {
		// Faking due to unreal taskpool or overload condition.
		// In the case of overload it's possible that some free space could exist
		//  half-way through, but short of very high counts or very long tasks, the taskpool itself will still be totally flooded.
		for (int i = 0; i < count; i++)
			func(ctx + (i * size));
	} else {
		taskpool_job job;
		job.func = func;
		job.ctx = ctx;
		for (int i = 0; i < count; i++) {
			taskpool_new_job(pool, job);
			ctx += size;
		}
		taskpool_new_cmd(pool, TASKPOOL_QUEUE_NOP);
	}
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
	if (pool->workers) {
		taskpool_new_cmd(pool, TASKPOOL_QUEUE_WAIT);
		taskpool_new_cmd(pool, TASKPOOL_QUEUE_NOP);
		while (!oscore_event_wait_until(pool->waitover, oscore_udate() + 1000)) {
			// ... You know, for added 'in case the wait wasn't enough'
			oscore_task_yield();
		}
	}
}

void taskpool_destroy(taskpool* pool) {
	// This command cleans up the ->whead, so we can just ignore the threads from now on, I hope?
	// ... or do we need to join them?
	if (pool->workers) {
		taskpool_new_cmd(pool, TASKPOOL_QUEUE_SHUTDOWN);
		for (int i = 0; i < pool->workers; i++)
			oscore_task_join(pool->tasks[i]);
	}
	// LEAK: Not bothering to handle freelist for now, want to get perf. stats first
	oscore_event_free(pool->waitover);
	oscore_event_free(pool->incoming);
	free(pool->tasks);
	free(pool);
	pool = NULL;
}
