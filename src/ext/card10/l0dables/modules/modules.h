#ifndef MODULES_H
#define MODULES_H

#include "FreeRTOS.h"
#include "semphr.h"

#include <stdint.h>
#include <stdbool.h>

/* ---------- Dispatcher --------------------------------------------------- */
void vApiDispatcher(void *pvParameters);
extern SemaphoreHandle_t api_mutex;
extern TaskHandle_t dispatcher_task_id;

/* ---------- Hardware Init & Reset ---------------------------------------- */
int hardware_early_init(void);
int hardware_init(void);
int hardware_reset(void);

/* ---------- Lifecycle ---------------------------------------------------- */
void vLifecycleTask(void *pvParameters);
void return_to_menu(void);

/* ---------- Serial ------------------------------------------------------- */
#define SERIAL_READ_BUFFER_SIZE 128
void vSerialTask(void *pvParameters);
void serial_enqueue_char(char chr);

/* ---------- LED Animation / Personal States ------------------------------ */
#define PERSONAL_STATE_LED 14
void vLedTask(void *pvParameters);
int personal_state_enabled();

/* ---------- PMIC --------------------------------------------------------- */
void vPmicTask(void *pvParameters);

/* Critical battery voltage */
#define BATTERY_CRITICAL   3.40f

enum pmic_amux_signal {
	PMIC_AMUX_CHGIN_U     = 0x1,
	PMIC_AMUX_CHGIN_I     = 0x2,
	PMIC_AMUX_BATT_U      = 0x3,
	PMIC_AMUX_BATT_CHG_I  = 0x4,
	PMIC_AMUX_BATT_DIS_I  = 0x5,
	PMIC_AMUX_BATT_NULL_I = 0x6,
	PMIC_AMUX_THM_U       = 0x7,
	PMIC_AMUX_TBIAS_U     = 0x8,
	PMIC_AMUX_AGND_U      = 0x9,
	PMIC_AMUX_SYS_U       = 0xA,
	_PMIC_AMUX_MAX,
};

/*
 * Read a value from the PMIC's AMUX.  The result is already converted into its
 * proper unit.  See the MAX77650 datasheet for details.
 */
int pmic_read_amux(enum pmic_amux_signal sig, float *result);


/* ---------- BLE ---------------------------------------------------------- */
void vBleTask(void *pvParameters);
bool ble_shall_start(void);
void ble_uart_write(uint8_t *pValue, uint8_t len);

/* ---------- Hardware (Peripheral) Locks ---------------------------------- */
void hwlock_init(void);

enum hwlock_periph {
	HWLOCK_I2C = 0,
	HWLOCK_ADC,
    HWLOCK_LED,
	_HWLOCK_MAX,
};

int hwlock_acquire(enum hwlock_periph p, TickType_t wait);
int hwlock_release(enum hwlock_periph p);

/* ---------- Display ------------------------------------------------------ */
/* Forces an unlock of the display. Only to be used in Epicardium */
void disp_forcelock();

#endif /* MODULES_H */
