#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <inttypes.h>

#include <stdio.h>

#define BUSY_WAIT 1
#define USEC_CONST 1000 * 1000
#define STACK_DEPTH 16384
#define TASK_PRIORITY configMAX_PRIORITIES -1
#define N_CPUCORES 1
// Set to 0 for handling this externally
#define USE_TASKS 0
// Ticks is waaay faster on some devices
#define USE_TICKS 1

static void sled_task(void* pvParameters) {
	if (sled_main(0, NULL)) {
		puts("fail\n");
	}
	puts("success\n");
}

// Main entry point.
// We can choose to pin to a core. This makes sense if your chip has more than 1 core.
// So you can pin SLED to one core and use the other cores for stuff like WiFi, Webserver, etc...
// Use core = -1 to not pin
// Set USE_TASKS to false to omit tasks
void user_main(void* pvParameters, int core) {
	if (!USE_TASKS) {
		sled_task(NULL);
		return;
	}

	if(xTaskGetSchedulerState() == 1)
		vTaskStartScheduler();
	if (core < 0 || core >= oscore_ncpus()) {
		xTaskCreate(sled_task, "sled_main", STACK_DEPTH, pvParameters, TASK_PRIORITY, NULL);
	} else {
		xTaskCreatePinnedToCore(sled_task, "sled_main", STACK_DEPTH, pvParameters, TASK_PRIORITY, NULL, core);
	}
}

// -- event
// Return the current tasks handle.
//
// In this implementation, we use semaphores.
// Taken is the default state, as we can wait on getting the lock.
// Signalling is done by giving the lock, also known as unlocking.

oscore_event oscore_event_new(void) {
	return xSemaphoreCreateBinary(); // starts in taken state
}

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	// If desired_usec is 0, it is used to simply clear the event.
	if (desired_usec == 0) {
		xSemaphoreTake(ev, 0); // Only take when not taken.
		return 0;
	}

	oscore_time waketick = oscore_udate();
	if (waketick >= desired_usec) {
		printf("timer is late, waketick: %" PRIu64 ", desired: %" PRIu64 "\n", waketick, desired_usec);
		return waketick;
	}

	TickType_t wait = BUSY_WAIT ? 0 : (TickType_t)((desired_usec-waketick) / 1000) / portTICK_PERIOD_MS;
	while (oscore_udate() < desired_usec) {
		if (xSemaphoreTake(ev, wait) == pdTRUE) {
			return 1;
		}
	}
	return 0;
}

void oscore_event_signal(oscore_event ev) {
	printf("Signaling.\n");
	xSemaphoreGive(ev);
}

void oscore_event_free(oscore_event ev) {
	vSemaphoreDelete(ev);
}

// Time keeping.
// FreeRTOS provides a tick count, but it's less accurate.
oscore_time oscore_udate(void) {
	if (USE_TICKS) {
		return (USEC_CONST / configTICK_RATE_HZ) * (oscore_time)xTaskGetTickCount();
	}
	oscore_task_yield();
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (T_SECOND * tv.tv_sec + tv.tv_usec);
}

// Below: Stubs and untestet stuff. Danger zone!

// -- mutex
oscore_mutex oscore_mutex_new(void) {
	SemaphoreHandle_t handle =  xSemaphoreCreateBinary();
	xSemaphoreGive(handle); // freertos semaphores start taken, "normal" *nix mutexes do not.
	return handle;
}

void oscore_mutex_lock(oscore_mutex m) {
	xSemaphoreTake(m, portMAX_DELAY);
}

void oscore_mutex_unlock(oscore_mutex m) {
	xSemaphoreGive(m);
}

void oscore_mutex_free(oscore_mutex m) {
	vSemaphoreDelete(m);
}

int oscore_ncpus(void) {
#ifdef ESP32
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	return chip_info.cores;
#endif
	return N_CPUCORES;
}


oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx) {
	TaskHandle_t xHandle = NULL;
	xTaskCreate((void *) func, name, STACK_DEPTH, ctx, TASK_PRIORITY, &xHandle);
	return xHandle;
}

void oscore_task_setprio(oscore_task task, int prio) {
	if (prio < 0) {
		prio = 0;
	} else if (prio > configMAX_PRIORITIES -1) {
		prio = configMAX_PRIORITIES -1;
	}
	vTaskPrioritySet(task, prio);
}

void oscore_task_yield(void) {
	taskYIELD();
};

void oscore_task_pin(oscore_task task, int cpu) {}

void* oscore_task_join(oscore_task task) {
	if (task != NULL) {
		vTaskDelete(task);
	}
	free(task);
	return 0;
};
