#include "epicardium.h"
#include "modules/modules.h"
#include "modules/log.h"

#include "card10.h"
#include "pmic.h"
#include "MAX77650-Arduino-Library.h"

#include "max32665.h"
#include "mxc_sys.h"
#include "mxc_pins.h"
#include "adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <stdio.h>

/* Task ID for the pmic handler */
static TaskHandle_t pmic_task_id = NULL;

enum {
	/* An irq was received, probably the power button */
	PMIC_NOTIFY_IRQ = 1,
	/* The timer has ticked and we should check the battery voltage again */
	PMIC_NOTIFY_MONITOR = 2,
};

void pmic_interrupt_callback(void *_)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if (pmic_task_id != NULL) {
		xTaskNotifyFromISR(
			pmic_task_id,
			PMIC_NOTIFY_IRQ,
			eSetBits,
			&xHigherPriorityTaskWoken
		);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

int pmic_read_amux(enum pmic_amux_signal sig, float *result)
{
	int ret = 0;

	if (sig > _PMIC_AMUX_MAX) {
		return -EINVAL;
	}

	ret = hwlock_acquire(HWLOCK_ADC, pdMS_TO_TICKS(100));
	if (ret < 0) {
		return ret;
	}
	ret = hwlock_acquire(HWLOCK_I2C, pdMS_TO_TICKS(100));
	if (ret < 0) {
		return ret;
	}

	/* Select the correct channel for this measurement.  */
	MAX77650_setMUX_SEL(sig);

	/*
	 * According to the datasheet, the voltage will stabilize within 0.3us.
	 * Just to be sure, we'll wait a little longer.  In the meantime,
	 * release the I2C mutex.
	 */
	hwlock_release(HWLOCK_I2C);
	vTaskDelay(pdMS_TO_TICKS(5));
	ret = hwlock_acquire(HWLOCK_I2C, pdMS_TO_TICKS(100));
	if (ret < 0) {
		return ret;
	}

	uint16_t adc_data;
	ADC_StartConvert(ADC_CH_0, 0, 0);
	ADC_GetData(&adc_data);

	/* Turn MUX back to neutral so it does not waste power.  */
	MAX77650_setMUX_SEL(sig);

	/* Convert ADC measurement to SI Volts */
	float adc_voltage = (float)adc_data / 1023.0f * 1.22f;

	/*
	 * Convert value according to PMIC formulas (Table 7)
	 */
	switch (sig) {
	case PMIC_AMUX_CHGIN_U:
		*result = adc_voltage / 0.167f;
		break;
	case PMIC_AMUX_CHGIN_I:
		*result = adc_voltage / 2.632f;
		break;
	case PMIC_AMUX_BATT_U:
		*result = adc_voltage / 0.272f;
		break;
	case PMIC_AMUX_BATT_CHG_I:
		*result = adc_voltage / 1.25f;
		break;
	case PMIC_AMUX_BATT_NULL_I:
	case PMIC_AMUX_THM_U:
	case PMIC_AMUX_TBIAS_U:
	case PMIC_AMUX_AGND_U:
		*result = adc_voltage;
		break;
	case PMIC_AMUX_SYS_U:
		*result = adc_voltage / 0.26f;
		break;
	default:
		ret = -EINVAL;
	}

	hwlock_release(HWLOCK_I2C);
	hwlock_release(HWLOCK_ADC);
	return ret;
}

/*
 * Read the interrupt flag register and handle all interrupts which the PMIC has
 * sent.  In most cases this will be the buttons.
 */
static void
pmic_poll_interrupts(TickType_t *button_start_tick, TickType_t duration)
{
	while (hwlock_acquire(HWLOCK_I2C, pdMS_TO_TICKS(500)) < 0) {
		LOG_WARN("pmic", "Failed to acquire I2C. Retrying ...");
		xTaskNotify(pmic_task_id, PMIC_NOTIFY_IRQ, eSetBits);
		return;
	}

	uint8_t int_flag = MAX77650_getINT_GLBL();
	hwlock_release(HWLOCK_I2C);

	if (int_flag & MAX77650_INT_nEN_F) {
		/* Button was pressed */
		*button_start_tick = xTaskGetTickCount();
	}
	if (int_flag & MAX77650_INT_nEN_R) {
		/* Button was released */
		*button_start_tick = 0;
		if (duration < pdMS_TO_TICKS(400)) {
			return_to_menu();
		} else {
			LOG_WARN("pmic", "Resetting ...");
			card10_reset();
		}
	}

	/* TODO: Remove when all interrupts are handled */
	if (int_flag & ~(MAX77650_INT_nEN_F | MAX77650_INT_nEN_R)) {
		LOG_WARN("pmic", "Unhandled PMIC Interrupt: %x", int_flag);
	}
}

__attribute__((noreturn)) static void pmic_die(float u_batt)
{
	/* Stop core 1 */
	core1_stop();

	/* Grab the screen */
	disp_forcelock();

	/* Draw an error screen */
	epic_disp_clear(0x0000);

	epic_disp_print(0, 0, " Battery", 0xffff, 0x0000);
	epic_disp_print(0, 20, " critical", 0xffff, 0x0000);
	epic_disp_print(0, 40, "  !!!!", 0xffff, 0x0000);
	epic_disp_update();

	/* Vibrate violently */
	epic_vibra_set(true);

	/* Wait a bit */
	for (int i = 0; i < 50000000; i++)
		__NOP();

	LOG_WARN("pmic", "Poweroff");
	MAX77650_setSFT_RST(0x2);

	while (1)
		__WFI();
}

/*
 * Check the battery voltage.  If it drops too low, turn card10 off.
 */
static void pmic_check_battery()
{
	float u_batt;
	int res;

	res = pmic_read_amux(PMIC_AMUX_BATT_U, &u_batt);
	if (res < 0) {
		LOG_ERR("pmic", "Failed reading battery voltage: %d", res);
		return;
	}

	LOG_DEBUG(
		"pmic",
		"Battery is at %d.%03d V",
		(int)u_batt,
		(int)(u_batt * 1000.0) % 1000
	);

	if (u_batt < BATTERY_CRITICAL) {
		pmic_die(u_batt);
	}
}

/*
 * API-call for battery voltage
 */
int epic_read_battery_voltage(float *result)
{
	return pmic_read_amux(PMIC_AMUX_BATT_U, result);
}

static StaticTimer_t pmic_timer_data;
static void vPmicTimerCb(TimerHandle_t xTimer)
{
	/*
	 * Tell the PMIC task to check the battery again.
	 */
	xTaskNotify(pmic_task_id, PMIC_NOTIFY_MONITOR, eSetBits);
}

void vPmicTask(void *pvParameters)
{
	pmic_task_id = xTaskGetCurrentTaskHandle();

	ADC_Init(0x9, NULL);
	GPIO_Config(&gpio_cfg_adc0);

	TickType_t button_start_tick = 0;

	pmic_check_battery();

	TimerHandle_t pmic_timer = xTimerCreateStatic(
		"PMIC Timer",
		pdMS_TO_TICKS(60 * 1000),
		pdTRUE,
		NULL,
		vPmicTimerCb,
		&pmic_timer_data
	);
	if (pmic_timer == NULL) {
		LOG_CRIT("pmic", "Could not create timer.");
		vTaskDelay(portMAX_DELAY);
	}
	xTimerStart(pmic_timer, 0);

	while (1) {
		uint32_t reason;
		if (button_start_tick == 0) {
			reason = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		} else {
			reason = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
		}

		TickType_t duration = xTaskGetTickCount() - button_start_tick;

		if (button_start_tick != 0 && duration > pdMS_TO_TICKS(1000)) {
			LOG_WARN("pmic", "Poweroff");
			MAX77650_setSFT_RST(0x2);
		}

		if (reason & PMIC_NOTIFY_IRQ) {
			pmic_poll_interrupts(&button_start_tick, duration);
		}

		if (reason & PMIC_NOTIFY_MONITOR) {
			pmic_check_battery();
		}
	}
}
