#ifndef __OSCORE_INCLUDED__
#define __OSCORE_INCLUDED__
#include "types.h"

typedef void * oscore_event;
// Create a new event. Failure results in assertion failed.
oscore_event oscore_event_new(void);
// NOTE: usecs == 0 is useful to clear the event.
// Returns 1 if the wait was undisturbed, else 0.
int oscore_event_wait(oscore_event ev, ulong usecs);
// Signal the event.
void oscore_event_signal(oscore_event ev);
void oscore_event_free(oscore_event ev);

typedef void * oscore_mutex;
oscore_mutex oscore_mutex_new(void);
void oscore_mutex_free(oscore_mutex ev);
void oscore_mutex_lock(oscore_mutex ev);
void oscore_mutex_unlock(oscore_mutex ev);

#endif
