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

static taskpool_queue_object * taskpool_alloc_obj(taskpool * pool) {
	// This is where you would stick 'freelist attempt 2'
	taskpool_queue_object * tqo = malloc(sizeof(taskpool_queue_object));
	assert(tqo);
	return tqo;
}

static void taskpool_free_obj(taskpool * pool, taskpool_queue_object * obj) {
	free(obj);
}


static void * taskpool_function(void* ctx) {
	taskpool* pool = (taskpool*) ctx;

	// Relaxed works for this because it's just picking a unique worker id,
	//  so long as worker IDs do not go out of bounds (thread overcreation somehow?) this is fine
	int workerid = atomic_fetch_add_explicit(&(pool->workeridatomic), 1, memory_order_relaxed);

	taskpool_queue_object * qobj = atomic_load(&(pool->whead));
	taskpool_job job;
	taskpool_queue_object * qobjnxt;
	while (qobj) {
		byte objtype = qobj->type;
		// Shutdown ignores the wait-for-next.
		qobjnxt = 0;
		if (objtype != TASKPOOL_QUEUE_SHUTDOWN) {
			while (!(qobjnxt = atomic_load_explicit(&(qobj->next), memory_order_acquire))) {
				// Burn CPU
				oscore_task_yield();
				oscore_event_wait_until(pool->incoming[workerid], oscore_udate() + 1000);
			}
#ifdef TASKPOOL_DEBUG_ALLTHETHINGS
			printf("Worker %i doing task execute\n", workerid);
#endif
		}
		if (objtype == TASKPOOL_QUEUE_JOB)
			job = qobj->jobdata;
		// This may doom the object to being deleted, so don't read from here on in
		int rc = atomic_fetch_sub_explicit(&(qobj->refcount), 1, memory_order_relaxed);
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
			int num = atomic_fetch_sub_explicit(&(pool->usage), 1, memory_order_relaxed);
#ifdef TASKPOOL_DEBUG_ALLTHETHINGS
			printf("Reducing usage by 1 now %i\n", num);
#endif
			taskpool_free_obj(pool, qobj);
		}
		// Shutdown has a next of 0
		qobj = qobjnxt;
	}
	return 0;
}

// Checked at submission
static int taskpool_overloaded(taskpool * pool) {
	int usage = atomic_load_explicit(&(pool->usage), memory_order_relaxed);
	return usage >= TASKPOOL_MAX_USAGE;
}

static void taskpool_inject_obj(taskpool * pool, taskpool_queue_object * obj) {
	int num = atomic_fetch_add_explicit(&(pool->usage), 1, memory_order_relaxed);
#ifdef TASKPOOL_DEBUG_ALLTHETHINGS
	printf("Increasing usage by 1 now %i\n", num);
#endif
	// Now for the writing side of things.
	taskpool_queue_object * t2 = atomic_exchange_explicit(&(pool->whead), obj, memory_order_acq_rel);
	atomic_store_explicit(&(t2->next), obj, memory_order_release);
	for (int i = 0; i < pool->workers; i++)
		oscore_event_signal(pool->incoming[i]);
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
	pool->incoming = calloc(sizeof(oscore_event), workers);

	pool->tasks = calloc(sizeof(oscore_task), workers);
	assert(pool->tasks);

	taskpool_queue_object * tqo = malloc(sizeof(taskpool_queue_object));
	assert(tqo);
	tqo->type = TASKPOOL_QUEUE_NOP;
	tqo->refcount = pool->workers;
	tqo->next = 0;
	pool->whead = tqo;

	// -- Do this last. It's thread creation. --

	int cpus_are_workers = oscore_ncpus() == workers;

	// If the oscore has NO thread support, not even faking, we still want basic GFX modules that use taskpool to not break.
	// So if pool->workers is 0, then we're just pretending.
	pool->workers = 0;
	for (int i = 0; i < workers; i++) {
		// Assigned worker IDs cannot exceed {0 .. i - 1} so long as the amount of pool tasks == i,
		//  so this is fine.
		pool->incoming[i] = oscore_event_new();
		pool->tasks[i] = oscore_task_create(pool_name, taskpool_function, pool);
		if (pool->tasks[i]) {
			oscore_task_setprio(pool->tasks[i], TPRIO_LOW);
			if (cpus_are_workers)
				oscore_task_pin(pool->tasks[i], i);
			pool->workers++;
		} else {
			// Creating event failed, don't continue to try & remove event.
			oscore_event_free(pool->incoming[i]);
			break;
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
		for (int i = 0; i < pool->workers; i++) {
			oscore_task_join(pool->tasks[i]);
			oscore_event_free(pool->incoming[i]);
		}
	}
	// LEAK: Not bothering to handle freelist for now, want to get perf. stats first
	oscore_event_free(pool->waitover);
	free(pool->incoming);
	free(pool->tasks);
	free(pool);
	pool = NULL;
}
