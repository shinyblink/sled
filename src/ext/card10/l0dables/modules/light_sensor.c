#include "epicardium.h"
#include "modules/log.h"
#include "modules/modules.h"

#include "mxc_config.h"
#include "led.h"
#include "adc.h"
#include "gpio.h"

#include "FreeRTOS.h"
#include "timers.h"

#define READ_FREQ pdMS_TO_TICKS(100)

static uint16_t last_value;
static TimerHandle_t poll_timer;
static StaticTimer_t poll_timer_buffer;

static int light_sensor_init()
{
	const sys_cfg_adc_t sys_adc_cfg =
		NULL; /* No system specific configuration needed. */
	if (ADC_Init(0x9, &sys_adc_cfg) != E_NO_ERROR) {
		return -EINVAL;
	}
	GPIO_Config(&gpio_cfg_adc7);
	return 0;
}

static void readAdcCallback()
{
	if (hwlock_acquire(HWLOCK_ADC, 0) != 0) {
		/* Can't do much about this here ... Retry next time */
		return;
	}

	ADC_StartConvert(ADC_CH_7, 0, 0);
	ADC_GetData(&last_value);

	hwlock_release(HWLOCK_ADC);
}

int epic_light_sensor_run()
{
	int ret = 0;

	if (hwlock_acquire(HWLOCK_ADC, pdMS_TO_TICKS(500)) != 0) {
		return -EBUSY;
	}

	light_sensor_init();

	if (!poll_timer) {
		poll_timer = xTimerCreateStatic(
			"light_sensor_adc",
			READ_FREQ,
			pdTRUE,
			NULL,
			readAdcCallback,
			&poll_timer_buffer
		);
		// since &poll_timer_buffer is not NULL, xTimerCreateStatic should allways succeed, so
		// we don't need to check for poll_timer being NULL.
	}
	if (xTimerIsTimerActive(poll_timer) == pdFALSE) {
		if (xTimerStart(poll_timer, 0) != pdPASS) {
			ret = -EBUSY;
		}
	}

	hwlock_release(HWLOCK_ADC);
	return ret;
}

int epic_light_sensor_stop()
{
	if (!poll_timer || xTimerIsTimerActive(poll_timer) == pdFALSE) {
		// timer wasn't running (or never started), just silently pass
		return 0;
	}

	if (xTimerStop(poll_timer, 0) != pdPASS) {
		return -EBUSY;
	}

	return 0;
}

int epic_light_sensor_get(uint16_t *value)
{
	if (!poll_timer || !xTimerIsTimerActive(poll_timer)) {
		return -ENODATA;
	}
	*value = last_value;
	return 0;
}
