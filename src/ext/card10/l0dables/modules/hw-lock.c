#include "modules/log.h"
#include "modules/modules.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <errno.h>

static StaticSemaphore_t hwlock_mutex_data[_HWLOCK_MAX];
static SemaphoreHandle_t hwlock_mutex[_HWLOCK_MAX];
/* Which task is holding the lock */
static TaskHandle_t hwlock_tasks[_HWLOCK_MAX];

void hwlock_init(void)
{
	for (int i = 0; i < _HWLOCK_MAX; i++) {
		hwlock_mutex[i] =
			xSemaphoreCreateMutexStatic(&hwlock_mutex_data[i]);
	}
}

int hwlock_acquire(enum hwlock_periph p, TickType_t wait)
{
	if (p >= _HWLOCK_MAX) {
		return -EINVAL;
	}

	if (xSemaphoreTake(hwlock_mutex[p], wait) != pdTRUE) {
		LOG_WARN("hwlock", "Lock %u is busy.", p);
		return -EBUSY;
	}

	hwlock_tasks[p] = xTaskGetCurrentTaskHandle();

	return 0;
}

int hwlock_release(enum hwlock_periph p)
{
	int ret = 0;

	if (p >= _HWLOCK_MAX) {
		return -EINVAL;
	}

	if (hwlock_tasks[p] != xTaskGetCurrentTaskHandle()) {
		LOG_ERR("hwlock",
			"Lock %u is released by task \"%s\" while it was acquired by \"%s\"",
			p,
			pcTaskGetName(NULL),
			pcTaskGetName(hwlock_tasks[p]));
		ret = -EACCES;
	}

	if (xSemaphoreGive(hwlock_mutex[p]) != pdTRUE) {
		LOG_ERR("hwlock", "Lock %u not released correctly.", p);
		ret = -EINVAL;
	}

	return ret;
}
