#include "epicardium.h"
#include "leds.h"
#include "modules.h"

#include <math.h>

uint8_t _personal_state_enabled   = 0;
uint8_t personal_state            = STATE_NONE;
uint8_t personal_state_persistent = 0;

int led_animation_ticks = 0;
int led_animation_state = 0;

int personal_state_enabled()
{
	return _personal_state_enabled;
}

int epic_personal_state_set(uint8_t state, bool persistent)
{
	if (state < STATE_NONE || state > STATE_CAMP)
		return -EINVAL;

	led_animation_state = 0;
	led_animation_ticks = 0;
	personal_state      = state;

	uint8_t was_enabled = _personal_state_enabled;

	_personal_state_enabled   = (state != STATE_NONE);
	personal_state_persistent = persistent;

	if (was_enabled && !_personal_state_enabled) {
		while (hwlock_acquire(HWLOCK_LED, pdMS_TO_TICKS(1)) < 0) {
			vTaskDelay(pdMS_TO_TICKS(1));
		}

		leds_prep(PERSONAL_STATE_LED, 0, 0, 0);
		leds_update_power();
		leds_update();

		hwlock_release(HWLOCK_LED);
	}

	return 0;
}

int epic_personal_state_get()
{
	return personal_state;
}

int epic_personal_state_is_persistent()
{
	return personal_state_persistent;
}

void vLedTask(void *pvParameters)
{
	const int led_animation_rate = 1000 / 25; /* 25Hz -> 40ms*/
	while (1) {
		if (_personal_state_enabled) {
			while (hwlock_acquire(HWLOCK_LED, pdMS_TO_TICKS(1)) <
			       0) {
				vTaskDelay(pdMS_TO_TICKS(1));
			}

			led_animation_ticks++;
			if (personal_state == STATE_NO_CONTACT) {
				leds_prep(PERSONAL_STATE_LED, 255, 0, 0);
			} else if (personal_state == STATE_CHAOS) {
				if (led_animation_state == 0) {
					leds_prep(
						PERSONAL_STATE_LED, 0, 0, 255
					);
					if (led_animation_ticks >
					    (200 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 1;
					}
				} else if (led_animation_state == 1) {
					leds_prep(PERSONAL_STATE_LED, 0, 0, 0);
					if (led_animation_ticks >
					    (300 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 2;
					}
				} else if (led_animation_state == 2) {
					leds_prep(
						PERSONAL_STATE_LED, 0, 0, 255
					);
					if (led_animation_ticks >
					    (1000 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 3;
					}
				} else if (led_animation_state == 3) {
					leds_prep(PERSONAL_STATE_LED, 0, 0, 0);
					if (led_animation_ticks >
					    (300 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 0;
					}
				}
			} else if (personal_state == STATE_COMMUNICATION) {
				if (led_animation_state == 0) {
					leds_prep(
						PERSONAL_STATE_LED, 255, 255, 0
					);
					if (led_animation_ticks >
					    (1000 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 1;
					}
				} else if (led_animation_state == 1) {
					leds_prep(PERSONAL_STATE_LED, 0, 0, 0);
					if (led_animation_ticks >
					    (300 / led_animation_rate)) {
						led_animation_ticks = 0;
						led_animation_state = 0;
					}
				}
			} else if (personal_state == STATE_CAMP) {
				leds_prep_hsv(
					PERSONAL_STATE_LED,
					120.0f,
					1.0f,
					fabs(sin(
						led_animation_ticks /
						(float)(1000 /
							led_animation_rate))));
			}
			leds_update_power();
			leds_update();

			hwlock_release(HWLOCK_LED);
		}

		vTaskDelay(led_animation_rate / portTICK_PERIOD_MS);
	}
}
