#include "modules/log.h"

#include "api/dispatcher.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define TIMEOUT pdMS_TO_TICKS(2000)

TaskHandle_t dispatcher_task_id;

static StaticSemaphore_t api_mutex_data;
SemaphoreHandle_t api_mutex = NULL;

/*
 * API dispatcher task.  This task will sleep until an API call is issued and
 * then wake up to dispatch it.
 */
void vApiDispatcher(void *pvParameters)
{
	api_mutex = xSemaphoreCreateMutexStatic(&api_mutex_data);

	LOG_DEBUG("dispatcher", "Ready.");
	while (1) {
		if (api_dispatcher_poll()) {
			if (xSemaphoreTake(api_mutex, TIMEOUT) != pdTRUE) {
				LOG_ERR("dispatcher", "API mutex blocked");
				continue;
			}
			api_dispatcher_exec();
			xSemaphoreGive(api_mutex);
		}
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}
