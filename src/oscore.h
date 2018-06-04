#ifndef __OSCORE_INCLUDED__
#define __OSCORE_INCLUDED__
#include "types.h"

typedef void * oscore_event;
// Create a new event. Failure results in assertion failed.
oscore_event oscore_event_new(void);
// NOTE: usecs == 0 is useful to clear the event.
// Returns 0 if the wait was undisturbed, anything not zero is an interrupt.
int oscore_event_wait_until(oscore_event ev, ulong desired_usec);
// Signal the event.
void oscore_event_signal(oscore_event ev);
void oscore_event_free(oscore_event ev);

// Get the real time in usecs,
// failing that, get the uptime.
ulong oscore_udate(void);

// Tasks
// While the "tasks" are usually multithreaded, they might be singletasking and therefore
// there must be a function to yield in such cases.
typedef void* oscore_task;
typedef void (*oscore_task_function)(void *);

oscore_task oscore_task_create(char* name, oscore_task_function func, void* ctx);
void oscore_task_yield(void);
void oscore_task_exit(int status);
int oscore_task_join(oscore_task task);

typedef void* oscore_mutex;
oscore_mutex oscore_mutex_new(void);
void oscore_mutex_free(oscore_mutex ev);
void oscore_mutex_lock(oscore_mutex ev);
void oscore_mutex_unlock(oscore_mutex ev);

#endif
