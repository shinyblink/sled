#include "epicardium.h"

#include "api/dispatcher.h"
#include "api/interrupt-sender.h"
#include "cdcacm.h"
#include "modules/filesystem.h"
#include "modules/log.h"
#include "modules/modules.h"
#include "modules/stream.h"

#include "card10.h"
#include "display.h"
#include "leds.h"
#include "pb.h"
#include "pmic.h"
#include "portexpander.h"

#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "trng.h"

/*
 * Early init is called at the very beginning and is meant for modules which
 * absolutely need to start as soon as possible.  hardware_early_init() blocks
 * which means code in here should be fast.
 */
int hardware_early_init(void)
{
	/*
	 * I2C bus for onboard peripherals (ie. PMIC, BMA400, BHI160, BME680,
	 * ...)
	 */
	I2C_Shutdown(MXC_I2C1_BUS0);
	I2C_Init(MXC_I2C1_BUS0, I2C_FAST_MODE, NULL);

#ifndef CARD10_DEBUG_CORE1
	/*
	 * SAO I2C bus
	 */
	I2C_Shutdown(MXC_I2C0_BUS0);
	I2C_Init(MXC_I2C0_BUS0, I2C_FAST_MODE, NULL);
#endif

	/*
	 * GPIO peripheral.
	 */
	GPIO_Init();

	/*
	 * PMIC (MAX77650)
	 */
	pmic_init();
	pmic_set_led(0, 0);
	pmic_set_led(1, 0);
	pmic_set_led(2, 0);

	/*
	 * Harmonic Board Portexpander
	 */
	portexpander_init();

	/*
	 * RNG
	 */
	TRNG_Init(NULL);

	/*
	 * Buttons
	 */
	PB_Init();

	/* Enable 32 kHz output */
	while (RTC_SquareWave(
		       MXC_RTC,
		       SQUARE_WAVE_ENABLED,
		       F_32KHZ,
		       NOISE_IMMUNE_MODE,
		       NULL) == E_BUSY
	)
		;

	/* If we don't have a valid time yet, set it to 2019-01-01 */
	if (RTC_GetSecond() < 1546300800UL) {
		epic_rtc_set_milliseconds(1546300800UL * 1000);
	}

	/*
	 * SPI for ECG
	 */
	const sys_cfg_spi_t spi17y_master_cfg = {
		.map = MAP_A,
		.ss0 = Enable,
		.ss1 = Disable,
		.ss2 = Disable,
	};

	if (SPI_Init(SPI0, 0, SPI_SPEED, spi17y_master_cfg) != 0) {
		LOG_ERR("init", "Error configuring SPI");
		while (1)
			;
	}

	/*
	 * The bootloader has already initialized the display, so we only need
	 * to do the bare minimum here (mostly the gfx datastructures).
	 */
	display_init_slim();

	/*
	 * RGB LEDs
	 */
	leds_init();

#ifdef CARD10_DEBUG_CORE1
	/*
	 * The SAO pins can be reconfigured for SWCLK2 and SWDIO2 which allows
	 * debugging core 1.  This feature can optionally be enabled at
	 * compile-time.
	 */
	LOG_WARN("init", "Core 1 Debugger Mode");
	static const gpio_cfg_t swclk = {
		PORT_0,
		PIN_7,
		GPIO_FUNC_ALT3,
		GPIO_PAD_NONE,
	};
	static const gpio_cfg_t swdio = {
		PORT_0,
		PIN_6,
		GPIO_FUNC_ALT3,
		GPIO_PAD_NONE,
	};

	GPIO_Config(&swclk);
	GPIO_Config(&swdio);
#endif /* CARD10_DEBUG_CORE1 */

	/*
	 * Enable SEV-ON-PEND which is needed for proper working of the FreeRTOS
	 * tickless idle sleep in Epicardium.
	 */
	SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

	/*
	 * USB-Serial
	 */
	if (cdcacm_init() < 0) {
		LOG_ERR("init", "USB-Serial unavailable");
	}

	/*
	 * Flash & FatFS
	 */
	fatfs_init();

	/*
	 * API Dispatcher & API Interrupts
	 */
	api_interrupt_init();
	api_dispatcher_init();

	/*
	 * Sensor streams
	 */
	stream_init();

	/*
	 * Hardware/Peripheral Locks
	 */
	hwlock_init();

	return 0;
}

/*
 * hardware_init() is called after the core has been bootstrapped and is meant
 * for less critical initialization steps.  Modules which initialize here should
 * be robust against a l0dable using their API before initialization is done.
 *
 * Ideally, acquire a lock in hardware_early_init() and release it in
 * hardware_init() once initialization is done.
 */
int hardware_init(void)
{
	/* Light Sensor */
	LOG_DEBUG("init", "Starting light sensor ...");
	epic_light_sensor_run();

	return 0;
}

/*
 * hardware_reset() is called whenever a new l0dable is started.  hardware_reset()
 * should bring all peripherals back into a known initial state.  This does not
 * necessarily mean resetting the peripheral entirely but hardware_reset()
 * should at least bring the API facing part of a peripheral back into the state
 * a fresh booted l0dable expects.
 */
int hardware_reset(void)
{
	/*
	 * API Dispatcher & API Interrupts
	 */
	api_interrupt_init();
	api_dispatcher_init();

	/* Personal State */
	const int personal_state_is_persistent =
		epic_personal_state_is_persistent();

	if (personal_state_is_persistent == 0) {
		epic_personal_state_set(STATE_NONE, 0);
	}

	/*
	 * LEDs
	 */
	if (personal_state_is_persistent) {
		epic_leds_clear_all(0, 0, 0);
	} else {
		leds_init();
	}
	epic_leds_set_rocket(0, 0);
	epic_leds_set_rocket(1, 0);
	epic_leds_set_rocket(2, 0);

	/*
	 * Display
	 */
	display_init_slim();

	return 0;
}
