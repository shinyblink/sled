#include "epicardium.h"
#include "api/dispatcher.h"
#include "api/interrupt-sender.h"
#include "modules/log.h"

#include "card10.h"

#include "max32665.h"
#include "sema.h"
#include "tmr.h"

static void __core1_init(void);

struct core1_info {
	/* Location of core1's interrupt vector table */
	volatile uintptr_t ivt_addr;
	/* Whether core 1 is ready for a new IVT */
	volatile bool ready;
};

/*
 * Information passing structure for controlling core 1.
 */
static volatile struct core1_info core1_info = {
	.ivt_addr = 0x00,
	.ready    = false,
};

/*
 * Minimal IVT needed for initial startup.  This IVT only contains the initial
 * stack pointer and reset-handler and is used to startup core 1.  Afterwards,
 * the payload's IVT is loaded into VTOR and used from then on.
 */
static uintptr_t core1_initial_ivt[] = {
	/* Initial Stack Pointer */
	0x20080000,
	/* Reset Handler */
	(uintptr_t)__core1_reset,
};

/*
 * Reset Handler
 *
 * Calls __core1_init() to reset & prepare the core for loading a new payload.
 */
__attribute__((naked)) void __core1_reset(void)
{
	/* Reset stack to MSP and set it to 0x20080000 */
	__asm volatile(
		"mov	r0, #0\n\t"
		"msr	control, r0\n\t"
		"mov	sp, %0\n\t"
		: /* No Outputs */
		: "r"(core1_initial_ivt[0])
		: "r0");

	/* Reset FPU */
	SCB->CPACR  = 0x00000000;
	FPU->FPDSCR = 0x00000000;
	FPU->FPCCR  = 0x00000000;
	__DSB();
	__ISB();

	__core1_init();
}

/*
 * Init core 1.  This function will reset the core and wait for a new IVT
 * address from Epicardium.  Once this address is received, it will start
 * execution with the supplied reset handler.
 */
void __core1_init(void)
{
	/*
	 * Clear any pending API interrupts.
	 */
	TMR_IntClear(MXC_TMR5);

	/*
	 * Reset Interrupts
	 *
	 * To ensure proper operation of the new payload, disable all interrupts
	 * and clear all pending ones.
	 */
	for (int i = 0; i < MXC_IRQ_EXT_COUNT; i++) {
		NVIC_DisableIRQ(i);
		NVIC_ClearPendingIRQ(i);
		NVIC_SetPriority(i, 0);
	}

	/*
	 * Check whether we catched the core during an interrupt.  If this is
	 * the case, try returning from the exception handler first and call
	 * __core1_reset() again in thread context.
	 */
	if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0) {
		/*
		 * Construct an exception frame so the CPU will jump back to our
		 * __core1_reset() function once we exit from the exception
		 * handler.
		 *
		 * To exit the exception, a special "EXC_RETURN" value is loaded
		 * into the link register and then branched to.
		 */
		__asm volatile(
			"ldr	r0, =0x41000000\n\t"
			"ldr	r1, =0\n\t"
			"push	{ r0 }\n\t" /* xPSR */
			"push	{ %0 }\n\t" /* PC */
			"push	{ %0 }\n\t" /* LR */
			"push	{ r1 }\n\t" /* R12 */
			"push	{ r1 }\n\t" /* R3 */
			"push	{ r1 }\n\t" /* R2 */
			"push	{ r1 }\n\t" /* R1 */
			"push	{ r1 }\n\t" /* R0 */

			"ldr	lr, =0xFFFFFFF9\n\t"
			"bx	lr\n\t"
			: /* No Outputs */
			: "r"((uintptr_t)__core1_reset)
			: "pc", "lr");

		/* unreachable */
		while (1)
			;
	}

	/* Wait for the IVT address */
	while (1) {
		while (SEMA_GetSema(_CONTROL_SEMAPHORE) == E_BUSY) {
		}

		__DMB();
		__ISB();

		/*
		 * The IVT address is reset to 0 by Epicardium before execution
		 * gets here.  Once a new address has been set, core 1 can use
		 * the new IVT.
		 */
		if (core1_info.ivt_addr != 0x00) {
			break;
		}

		/* Signal that we are ready for an IVT address */
		core1_info.ready = true;

		/*
		 * Reset the API interrupt so we never block Epicardium when it
		 * attempts to trigger an interrupt.
		 */
		API_CALL_MEM->int_id = (-1);

		SEMA_FreeSema(_CONTROL_SEMAPHORE);

		__WFE();
	}

	uintptr_t *ivt      = (uintptr_t *)core1_info.ivt_addr;
	core1_info.ivt_addr = 0x00;

	SEMA_FreeSema(_CONTROL_SEMAPHORE);

	/*
	 * Reset the call-flag before entering the payload so API calls behave
	 * properly.  This is necessary because epic_exec() will set the flag
	 * to "returning" on exit.
	 */
	API_CALL_MEM->call_flag = _API_FLAG_IDLE;

	/*
	 * Set the IVT
	 */
	SCB->VTOR = (uintptr_t)ivt;

	/*
	 * Clear any pending API interrupts.
	 */
	TMR_IntClear(MXC_TMR5);
	NVIC_ClearPendingIRQ(TMR5_IRQn);

	/*
	 * Jump to payload's reset handler
	 */
	__asm volatile(
		"ldr r0, %0\n\t"
		"blx r0\n\r"
		: /* No Outputs */
		: "m"(*(ivt + 1))
		: "r0");
}

void core1_boot(void)
{
	/*
	 * Boot using the initial IVT.  This will place core 1 into a loop,
	 * waiting for a payload.
	 */
	core1_start(&core1_initial_ivt);
}

void core1_trigger_reset(void)
{
	/* Signal core 1 that we intend to load a new payload. */
	api_interrupt_trigger(EPIC_INT_RESET);
}

void core1_wait_ready(void)
{
	/* Wait for the core to accept */
	while (1) {
		while (SEMA_GetSema(_CONTROL_SEMAPHORE) == E_BUSY) {
		}

		/*
		 * core 1 will set the ready flag once it is spinning in the
		 * above loop, waiting for a new IVT.
		 */
		if (core1_info.ready) {
			break;
		}

		SEMA_FreeSema(_CONTROL_SEMAPHORE);

		for (int i = 0; i < 10000; i++) {
		}
	}

	/*
	 * TODO: If the other core does not respond within a certain grace
	 * period, we need to force it into our desired state by overwriting
	 * all of its memory.  Yes, I don't like this method either ...
	 */
}

void core1_load(void *ivt, char *args)
{
	/* If the core is currently in an API call, reset it. */
	API_CALL_MEM->call_flag = _API_FLAG_IDLE;
	API_CALL_MEM->id        = 0;
	API_CALL_MEM->int_id    = (-1);

	api_prepare_args(args);

	core1_info.ivt_addr = (uintptr_t)ivt;
	core1_info.ready    = false;

	__DMB();
	__ISB();

	SEMA_FreeSema(_CONTROL_SEMAPHORE);

	__SEV();
	__WFE();
}
