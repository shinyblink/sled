#include "api/common.h"

/*
 * Initialize the API system.  This function *must* be called
 * before any API action can take place.
 */
int api_dispatcher_init();

/*
 * Check whether the other core requested a call.  If this function returns
 * true, the dispatcher should call api_dispatcher_exec() to actually dispatch
 * the call.  Consecutive calls to this function will return false.  Use
 * api_dispatcher_poll() if your need to recheck.
 */
bool api_dispatcher_poll_once();
bool api_dispatcher_poll();

/*
 * Attempt to dispatch a call, if one had been polled using
 * api_dispatcher_poll().  Will return 0 if no call was dispatched or the ID of
 * the dispatched call otherwise.
 */
api_id_t api_dispatcher_exec();

/*
 * Fill the API buffer with data for l0dable/pycardium startup.
 *
 * The data is a NULL-terminated string.
 */
void api_prepare_args(char *args);

/*********************************************************************
 *                         core 1 control                            *
 *********************************************************************/

/* Startup core1 into a state where it is ready to receive a payload. */
void core1_boot(void);

/* Reset core 1 into a state where it can accept a new payload */
void core1_trigger_reset(void);

/* Wait for core 1 to respond that it is ready for a new payload */
void core1_wait_ready(void);

/* Load a payload into core 1 */
void core1_load(void *ivt, char *args);

/* core 1 reset stub.  See epicardium/api/control.c for details. */
void __core1_reset(void);
