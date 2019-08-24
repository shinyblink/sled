#include "leds.h"
#include "pmic.h"
#include "FreeRTOS.h"
#include "task.h"
#include "epicardium.h"
#include "modules.h"

#include <stdbool.h>

//TODO: create smth like vTaskDelay(pdMS_TO_TICKS(//put ms here)) for us, remove blocking delay from /lib/leds.c to avoid process blocking

#define NUM_LEDS 15 /* Take from lib/card10/leds.c */

static void do_update()
{
	while (hwlock_acquire(HWLOCK_LED, pdMS_TO_TICKS(1)) < 0) {
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	leds_update_power();
	leds_update();

	hwlock_release(HWLOCK_LED);
}

void epic_leds_set(int led, uint8_t r, uint8_t g, uint8_t b)
{
	if (led == PERSONAL_STATE_LED && personal_state_enabled())
		return;

	leds_prep(led, r, g, b);
	do_update();
}

void epic_leds_set_hsv(int led, float h, float s, float v)
{
	if (led == PERSONAL_STATE_LED && personal_state_enabled())
		return;

	leds_prep_hsv(led, h, s, v);
	do_update();
}

void epic_leds_prep(int led, uint8_t r, uint8_t g, uint8_t b)
{
	if (led == PERSONAL_STATE_LED && personal_state_enabled())
		return;

	leds_prep(led, r, g, b);
}

void epic_leds_prep_hsv(int led, float h, float s, float v)
{
	if (led == PERSONAL_STATE_LED && personal_state_enabled())
		return;

	leds_prep_hsv(led, h, s, v);
}

void epic_leds_set_all(uint8_t *pattern_ptr, uint8_t len)
{
	uint8_t(*pattern)[3] = (uint8_t(*)[3])pattern_ptr;
	for (int i = 0; i < len; i++) {
		if (i == PERSONAL_STATE_LED && personal_state_enabled())
			continue;

		leds_prep(i, pattern[i][0], pattern[i][1], pattern[i][2]);
	}
	do_update();
}

void epic_leds_set_all_hsv(float *pattern_ptr, uint8_t len)
{
	float(*pattern)[3] = (float(*)[3])pattern_ptr;
	for (int i = 0; i < len; i++) {
		if (i == PERSONAL_STATE_LED && personal_state_enabled())
			continue;

		leds_prep_hsv(i, pattern[i][0], pattern[i][1], pattern[i][2]);
	}
	do_update();
}

void epic_leds_dim_top(uint8_t value)
{
	leds_set_dim_top(value);
	if (personal_state_enabled() == 0)
		leds_update();
}

void epic_leds_dim_bottom(uint8_t value)
{
	leds_set_dim_bottom(value);
	if (personal_state_enabled() == 0)
		leds_update();
}

void epic_leds_set_rocket(int led, uint8_t value)
{
	value = value > 31 ? 31 : value;
	pmic_set_led(led, value);
}

void epic_set_flashlight(bool power)
{
	leds_flashlight(power);
}

void epic_leds_update(void)
{
	do_update();
}

void epic_leds_set_powersave(bool eco)
{
	leds_powersave(eco);
}

void epic_leds_set_gamma_table(uint8_t rgb_channel, uint8_t gamma_table[256])
{
	leds_set_gamma_table(rgb_channel, gamma_table);
}

void epic_leds_clear_all(uint8_t r, uint8_t g, uint8_t b)
{
	for (int i = 0; i < NUM_LEDS; i++) {
		if (i == PERSONAL_STATE_LED && personal_state_enabled())
			continue;

		leds_prep(i, r, g, b);
	}

	do_update();
}
