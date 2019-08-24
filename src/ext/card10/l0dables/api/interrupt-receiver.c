#include "max32665.h"
#include "tmr.h"
#include "api/common.h"
#include "epicardium.h"

void __dispatch_isr(api_int_id_t);

/* Timer Interrupt used for control char notification */
void TMR5_IRQHandler(void)
{
	TMR_IntClear(MXC_TMR5);
	__dispatch_isr(API_CALL_MEM->int_id);
	API_CALL_MEM->int_id = (-1);
}

/* Reset Handler */
void __epic_isr_reset(void)
{
	API_CALL_MEM->int_id = (-1);
	API_CALL_MEM->reset_stub();
}
