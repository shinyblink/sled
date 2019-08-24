#pragma once
#include "epicardium.h"

#include <stdint.h>
#include <stdbool.h>

/*
 * Semaphore used for API synchronization.
 * TODO: Replace this with a LDREX/STREX based implementation
 */
#define _API_SEMAPHORE        0
#define _CONTROL_SEMAPHORE    1

/* Type of API IDs */
typedef uint32_t api_id_t;

#define _API_FLAG_IDLE        0
#define _API_FLAG_CALLING     1
#define _API_FLAG_RETURNED    2

/* Layout of the shared memory for API calls */
struct api_call_mem {
	/*
	 * Reset stub.  The reset stub is a small function provided by
	 * epicardium that should be called by a payload when receiving the
	 * reset interrupt.
	 */
	void (*reset_stub)();

	/*
	 * Flag for synchronization of API calls.  When this flag
	 * is set, the caller has issued a call and is waiting for
	 * the dispatcher to reset the flag.
	 */
	uint8_t call_flag;

	/* ID if the ongoing API call */
	api_id_t id;

	/* ID of the current interrupt */
	api_int_id_t int_id;

	/*
	 * Buffer for arguments/return value.  This buffer will be
	 * *overflown*, because there is guaranteed space behind it.
	 *
	 * TODO: Add a maximum bounds check
	 */
	uint8_t buffer[1];
};

/* TODO: Make this address part of the linker script */
static __attribute__((unused)) struct api_call_mem* API_CALL_MEM =
	(struct api_call_mem*)0x20080000;
