/*
 * FreeRTOS support functions
 */

#include "FreeRTOS.h"
#include "task.h"

#include "api/dispatcher.h"
#include "modules/log.h"

#include "card10.h"

extern TaskHandle_t dispatcher_task_id;

/*
 * This hook is called before FreeRTOS enters tickless idle.
 */
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

/*
 * This hook is called after FreeRTOS exits tickless idle.
 */
void post_idle_sleep(TickType_t xExpectedIdleTime)
{
	/* Check whether a new API call was issued. */
	if (api_dispatcher_poll_once()) {
		xTaskNotifyGive(dispatcher_task_id);
	}

	/*
	 * Do card10 house keeping. e.g. polling the i2c devices if they
	 * triggered an interrupt.
	 *
	 * TODO: Do this in a more task focused way (high/low ISR)
	 */
	card10_poll();
}

void vApplicationGetIdleTaskMemory(
	StaticTask_t **ppxIdleTaskTCBBuffer,
	StackType_t **ppxIdleTaskStackBuffer,
	uint32_t *pulIdleTaskStackSize
) {
	/*
	 * If the buffers to be provided to the Idle task are declared inside this
	 * function then they must be declared static - otherwise they will be allocated on
	 * the stack and so not exists after this function exits.
	 */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	/*
	 * Pass out a pointer to the StaticTask_t structure in which the Idle task's
	 * ktate will be stored.
	 */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/*
	 * Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	 * Note that, as the array is necessarily of type StackType_t,
	 * configMINIMAL_STACK_SIZE is specified in words, not bytes.
	 */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(
	StaticTask_t **ppxTimerTaskTCBBuffer,
	StackType_t **ppxTimerTaskStackBuffer,
	uint32_t *pulTimerTaskStackSize
) {
	/*
	 * If the buffers to be provided to the Timer task are declared inside
	 * this function then they must be declared static - otherwise they will
	 * be allocated on the stack and so not exists after this function
	 * exits.
	 */
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

	/*
	 * Pass out a pointer to the StaticTask_t structure in which the Timer
	 * task's state will be stored.
	 */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/*
	 * Pass out the size of the array pointed to by
	 * *ppxTimerTaskStackBuffer.  Note that, as the array is necessarily of
	 * type StackType_t, configMINIMAL_STACK_SIZE is specified in words, not
	 * bytes.
	 */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
	LOG_CRIT("rtos", "Task \"%s\" overflowed stack!", pcTaskName);
}
