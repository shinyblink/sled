// os_freertos: FreeRTOS-based stuff.
// Lots of stubs.

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#define STACK_DEPTH 4096
#define TASK_PRIORITY 10

// Main entry point.
static void sled_task(void *pvParameters) {
	sled_main(0, NULL);
}

void user_main(void) {
	if(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
		vTaskStartScheduler();

	xTaskCreate(&sled_task, "sled", STACK_DEPTH, NULL, 10, NULL);
}

// -- event
oscore_event oscore_event_new(void) {
	return NULL;
}

int oscore_event_wait_until(oscore_event ev, ulong desired_usec) {
	portTickType waketick = (desired_usec / 1000) / portTICK_RATE_MS;
	vTaskDelayUntil(&waketick, 0);
	return 0;
}

void oscore_event_signal(oscore_event ev) {
}

void oscore_event_free(oscore_event ev) {
}

// -- mutex
#define TOMUT(m) (* (SemaphoreHandle_t*) (m))
oscore_mutex oscore_mutex_new(void) {
	return calloc(1, sizeof(SemaphoreHandle_t));
}

void oscore_mutex_lock(oscore_mutex m) {
	xSemaphoreTake(TOMUT(m), portMAX_DELAY);
}

void oscore_mutex_unlock(oscore_mutex m) {
	xSemaphoreGive(TOMUT(m));
}

void oscore_mutex_free(oscore_mutex m) {
	free(m);
}
