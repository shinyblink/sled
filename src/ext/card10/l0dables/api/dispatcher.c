#include "api/dispatcher.h"

#include "max32665.h"
#include "sema.h"

#include <stdlib.h>
#include <string.h>

/* This function is defined by the generated dispatcher code */
void __api_dispatch_call(api_id_t id, void *buffer);

static volatile bool event_ready = false;

int api_dispatcher_init()
{
	int ret;

	ret = SEMA_Init(NULL);
	SEMA_FreeSema(_API_SEMAPHORE);
	API_CALL_MEM->reset_stub = __core1_reset;
	API_CALL_MEM->call_flag  = _API_FLAG_IDLE;
	API_CALL_MEM->id         = 0;
	API_CALL_MEM->int_id     = (-1);

	/*
	 * Enable TX events for both cores.
	 * TODO: Is this the right place?
	 */
	MXC_GCR->evten |=
		MXC_F_GCR_EVTEN_CPU0TXEVENT | MXC_F_GCR_EVTEN_CPU1TXEVENT;

	return ret;
}

bool api_dispatcher_poll_once()
{
	if (event_ready) {
		return false;
	}

	while (SEMA_GetSema(_API_SEMAPHORE) == E_BUSY) {
	}

	if (API_CALL_MEM->call_flag != _API_FLAG_CALLING) {
		SEMA_FreeSema(_API_SEMAPHORE);
		return false;
	}

	event_ready = true;
	return true;
}

bool api_dispatcher_poll()
{
	if (event_ready) {
		return true;
	}

	return api_dispatcher_poll_once();
}

api_id_t api_dispatcher_exec()
{
	if (!event_ready) {
		return 0;
	}

	api_id_t id = API_CALL_MEM->id;
	__api_dispatch_call(id, API_CALL_MEM->buffer);
	API_CALL_MEM->call_flag = _API_FLAG_RETURNED;

	event_ready = false;
	SEMA_FreeSema(_API_SEMAPHORE);

	/* Notify the caller that we returned */
	__SEV();
	__WFE();

	return id;
}

void api_prepare_args(char *args)
{
	/*
	 * The args are stored with an offset of 0x20 to make sure they won't
	 * collide with any integer return value of API calls like epic_exec().
	 */
	API_CALL_MEM->id = 0;
	for (int i = 0; i <= strlen(args); i++) {
		API_CALL_MEM->buffer[i + 0x20] = args[i];
	}
}
