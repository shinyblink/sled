#include "api/interrupt-sender.h"
#include "api/common.h"
#include "tmr_utils.h"

static bool int_enabled[EPIC_INT_NUM];

int api_interrupt_trigger(api_int_id_t id)
{
	if (id >= EPIC_INT_NUM) {
		return -EINVAL;
	}

	if (int_enabled[id]) {
		while (API_CALL_MEM->int_id != (-1))
			;

		API_CALL_MEM->int_id = id;
		TMR_TO_Start(MXC_TMR5, 1, 0);
	}
	return 0;
}

void api_interrupt_init(void)
{
	API_CALL_MEM->int_id = (-1);

	for (int i = 0; i < EPIC_INT_NUM; i++) {
		int_enabled[i] = false;
	}

	/* Reset interrupt is always enabled */
	int_enabled[EPIC_INT_RESET] = true;
}

int epic_interrupt_enable(api_int_id_t int_id)
{
	if (int_id >= EPIC_INT_NUM) {
		return -EINVAL;
	}

	int_enabled[int_id] = true;
	return 0;
}

int epic_interrupt_disable(api_int_id_t int_id)
{
	if (int_id >= EPIC_INT_NUM || int_id == EPIC_INT_RESET) {
		return -EINVAL;
	}

	int_enabled[int_id] = false;
	return 0;
}
