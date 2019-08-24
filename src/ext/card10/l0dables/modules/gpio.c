#include "epicardium.h"
#include "gpio.h"
#include "max32665.h"
#include "mxc_errors.h"

/*
 * Despite what the schematic (currently, 2019-08-18) says these are the correct
 * pins for wristband GPIO 1-4 (not 0-3 as the schematic states)
 */
static gpio_cfg_t gpio_configs[] = {
	[GPIO_WRISTBAND_1] = { PORT_0, PIN_21, GPIO_FUNC_OUT, GPIO_PAD_NONE },
	[GPIO_WRISTBAND_2] = { PORT_0, PIN_22, GPIO_FUNC_OUT, GPIO_PAD_NONE },
	[GPIO_WRISTBAND_3] = { PORT_0, PIN_29, GPIO_FUNC_OUT, GPIO_PAD_NONE },
	[GPIO_WRISTBAND_4] = { PORT_0, PIN_20, GPIO_FUNC_OUT, GPIO_PAD_NONE },
};

int epic_gpio_set_pin_mode(uint8_t pin, uint8_t mode)
{
	if (pin < GPIO_WRISTBAND_1 || pin > GPIO_WRISTBAND_4)
		return -EINVAL;

	gpio_cfg_t *cfg = &gpio_configs[pin];

	bool is_input  = (mode & GPIO_MODE_IN) == GPIO_MODE_IN;
	bool is_output = (mode & GPIO_MODE_OUT) == GPIO_MODE_OUT;

	// Pins can't be input and output at the same time.
	if (is_input && is_output)
		return -EINVAL;

	uint32_t func_value = 0;
	if (is_input)
		func_value |= GPIO_FUNC_IN;
	if (is_output)
		func_value |= GPIO_FUNC_OUT;

	uint32_t pad_value = 0;
	if (mode & GPIO_PULL_UP)
		pad_value |= GPIO_PAD_PULL_UP;
	if (mode & GPIO_PULL_DOWN)
		pad_value |= GPIO_PAD_PULL_DOWN;

	cfg->func = func_value;
	cfg->pad  = pad_value;

	if (GPIO_Config(cfg) != E_NO_ERROR)
		return -EINVAL;
	return 0;
}

int epic_gpio_get_pin_mode(uint8_t pin)
{
	if (pin < GPIO_WRISTBAND_1 || pin > GPIO_WRISTBAND_4)
		return -EINVAL;

	gpio_cfg_t *cfg = &gpio_configs[pin];
	int res         = 0;
	if ((cfg->func & GPIO_FUNC_IN) == GPIO_FUNC_IN)
		res |= GPIO_MODE_IN;
	if ((cfg->func & GPIO_FUNC_OUT) == GPIO_FUNC_OUT)
		res |= GPIO_MODE_OUT;
	if ((cfg->pad & GPIO_PAD_PULL_UP) == GPIO_PAD_PULL_UP)
		res |= GPIO_PULL_UP;
	if ((cfg->pad & GPIO_PAD_PULL_DOWN) == GPIO_PAD_PULL_DOWN)
		res |= GPIO_PULL_DOWN;

	return res;
}

int epic_gpio_write_pin(uint8_t pin, bool on)
{
	if (pin < GPIO_WRISTBAND_1 || pin > GPIO_WRISTBAND_4)
		return -EINVAL;

	gpio_cfg_t *cfg = &gpio_configs[pin];
	if ((cfg->func & GPIO_FUNC_IN) == GPIO_FUNC_IN)
		return -EINVAL;

	if (on)
		GPIO_OutSet(cfg);
	else
		GPIO_OutClr(cfg);

	return 0;
}

uint32_t epic_gpio_read_pin(uint8_t pin)
{
	if (pin < GPIO_WRISTBAND_1 || pin > GPIO_WRISTBAND_4)
		return -EINVAL;

	gpio_cfg_t *cfg = &gpio_configs[pin];
	if ((cfg->func & GPIO_FUNC_OUT) == GPIO_FUNC_OUT) {
		return GPIO_OutGet(cfg);
	} else if ((cfg->func & GPIO_FUNC_IN) == GPIO_FUNC_IN) {
		return GPIO_InGet(cfg);
	} else {
		return -EINVAL;
	}
}
