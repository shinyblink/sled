// Taskpool header
#ifndef __INCLUDED_TASKPOOL__
#define __INCLUDED_TASKPOOL__

#include "types.h"
#include "oscore.h"
#include <stddef.h>
#include <stdatomic.h>

// The new queue system plan is like this:
// Each thread has it's own read head, and are all attached to the same queue.
// A thread only executes an object under two conditions:
// 1. when it has found a 'next' object, which it retrieves *before execution*, and it will then advance to after execution
// 2. when the object is of type TASKPOOL_QUEUE_SHUTDOWN which will by definition never have a next object.
// The 'next' pointer MUST ONLY EVER BE SET ONCE, ATOMICALLY.
// The queue objects have a reference counter, which starts at the amount of workers, and is lowered via get-and-decrement on execution.
// Note that any other data needed for execution must be retrieved before execution.
// This requires that the queue be interleaved between 'NOP' objects and 'JOB' objects.
// For added efficiency, commands may specify many JOB objects followed by one NOP object.

// Execution details:
// 'TASKPOOL_QUEUE_NOP': Used as a spacer to ensure execution of the last object. No effects of its own.
// 'TASKPOOL_QUEUE_JOB': Executes a job. The old reference counter value (hence get-and-decrement) is checked against the total worker counter,
//   which is what it starts at in the first place. If it's the total worker counter - 1, then this thread was first and runs the job. Otherwise the job is ignored.
// 'TASKPOOL_QUEUE_SHUTDOWN': See the note above on immediate execution. The reference counter still applies and is used for the cleanup of the queue.
// 'TASKPOOL_QUEUE_WAIT': The deallocating thread must signal the waitover event.
#define TASKPOOL_QUEUE_NOP 0
#define TASKPOOL_QUEUE_JOB 1
#define TASKPOOL_QUEUE_SHUTDOWN 2
#define TASKPOOL_QUEUE_WAIT 3

#define TASKPOOL_MAX_USAGE 80

// #define TASKPOOL_DEBUG_ALLTHETHINGS

typedef struct {
	void (*func)(void*);
	void* ctx;
} taskpool_job;

typedef struct taskpool_queue_object {
	byte type;
	_Atomic(int) refcount;
	_Atomic(struct taskpool_queue_object*) next; // Write atomically, but with no conditions - access is controlled by the get-and-set in taskpool.
	taskpool_job jobdata;
} taskpool_queue_object;

typedef struct {
	// Amount of entries in tasks. If 0, then we're just pretending.
	int workers;
	// This starts at 0 and is incremented when worker threads start to allocate their place in the incoming array.
	_Atomic(int) workeridatomic;
	// Used to prevent OOM-by-overload. Atomically inc/dec'd by all involved threads as queue objects move through the system.
	_Atomic(int) usage;
	oscore_task * tasks;
	oscore_event waitover; // waitover is threads -> writer, incoming is basically a dummy but is signalled writer -> threads
	oscore_event * incoming; // This is an array of events, length equal to that of tasks. Workers = 0 behavior is dependent on calloc behavior.
	// The last queue object. Write with get-and-set.
	// This has to be done first because if the taskpool queue object was done first,
	//  the possibility would arise of another writer using a head that's deallocated.
	_Atomic(taskpool_queue_object*) whead;
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
