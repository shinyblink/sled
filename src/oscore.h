#ifndef __OSCORE_INCLUDED__
#define __OSCORE_INCLUDED__
#include "types.h"

typedef void * oscore_event;
// Create a new event. Failure results in assertion failed.
oscore_event oscore_event_new(void);
// NOTE: usecs == 0 is useful to clear the event.
// Returns 0 if the wait was undisturbed, anything not zero is an interrupt.
// NOTE: If multiple threads are waiting on the same event, it's implementation-dependent.
// Implementations are free to use a method that ensures only one thread, or all waiting threads, get interrupted.
int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec);
// Signal the event.
void oscore_event_signal(oscore_event ev);
void oscore_event_free(oscore_event ev);

// Get the real time in usecs,
// failing that, get the uptime.
oscore_time oscore_udate(void);

// Tasks
// While the "tasks" are usually multithreaded, they might be singletasking and therefore
// there must be a function to yield in such cases.
typedef void* oscore_task;
typedef void * (*oscore_task_function)(void *);

// Can return NULL for error, since frankly a thread not being executed is somewhat less ignorable.
oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx);
void oscore_task_yield(void);
void oscore_task_exit(void * status);
void * oscore_task_join(oscore_task task);
int oscore_ncpus(void);
void oscore_task_pin(oscore_task task, int cpu);

#define TPRIO_HIGH 1
#define TPRIO_NORMAL 0
#define TPRIO_LOW -1
void oscore_task_setprio(oscore_task task, int prio);

typedef void* oscore_mutex;
oscore_mutex oscore_mutex_new(void);
void oscore_mutex_free(oscore_mutex ev);
void oscore_mutex_lock(oscore_mutex ev);
void oscore_mutex_unlock(oscore_mutex ev);

#endif
