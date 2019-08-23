#include "gpio.h"
#include "FreeRTOS.h"
#include "timers.h"

static const gpio_cfg_t motor_pin = {
	PORT_0, PIN_8, GPIO_FUNC_OUT, GPIO_PAD_NONE
};

static TimerHandle_t vibra_timer;

void epic_vibra_set(int status)
{
	if (status) {
		GPIO_OutSet(&motor_pin);
	} else {
		GPIO_OutClr(&motor_pin);
	}
}

void vTimerCallback()
{
	epic_vibra_set(0);
}

void epic_vibra_vibrate(int millis)
{
	int ticks = millis * (configTICK_RATE_HZ / 1000);
	epic_vibra_set(1);
	vibra_timer =
		xTimerCreate("vibratimer", ticks, pdFALSE, 0, vTimerCallback);
	xTimerStart(vibra_timer, 0);
}
