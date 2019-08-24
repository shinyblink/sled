#include "epicardium.h"

#include <math.h>

#include "FreeRTOS.h"
#include "task.h"
#include "api/dispatcher.h"
#include "modules/log.h"


void user_main(void* pvParameters, int core);

extern TaskHandle_t dispatcher_task_id;

int main(void) {
	user_main(NULL, -1);
}

void post_idle_sleep(TickType_t xExpectedIdleTime)
{
	// TODO
}

void pre_idle_sleep(TickType_t xExpectedIdleTime)
{
	if (xExpectedIdleTime > 0) {
		/*
		 * WFE because the other core should be able to notify
		 * epicardium if it wants to issue an API call.
		 */

		/*
		 * TODO: Ensure this is actually correct and does not have any
		 * race conditions.
		 */
		if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
			__asm volatile("dsb" ::: "memory");
			__asm volatile("wfe");
			__asm volatile("isb");
		}
	}
}
//
///*
// * This hook is called after FreeRTOS exits tickless idle.
// */
//void post_idle_sleep(TickType_t xExpectedIdleTime)
//{
//	/* Check whether a new API call was issued. */
//	if (api_dispatcher_poll_once()) {
//		xTaskNotifyGive(dispatcher_task_id);
//	}
//
//	/*
//	 * Do card10 house keeping. e.g. polling the i2c devices if they
//	 * triggered an interrupt.
//	 *
//	 * TODO: Do this in a more task focused way (high/low ISR)
//	 */
//	card10_poll();
//}
//
