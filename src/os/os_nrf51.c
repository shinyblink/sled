// os_nrf51822: nRF51822 (see - micro:bit) chip
// based off of os_dummy.c, so just as many stubs
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "../types.h"
#include "../oscore.h"
#include "../main.h"
#include "../timers.h"
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// --- SECTION 1 - INIT, UART ---

// Let's begin with the basics.
// This vector table is common to all Cortex-M0 bare-metal programming,
// it's just in here because here's sort of a good place for it.

__asm__(
	".globl __stack_end__\n"
	".section .vectors\n"
	".word __stack_end__\n"
	".word __prestart\n"
	".fill 30, 4, 0x00000080\n"
	// this must be at 0x80
	"__interrupt:\n"
	"b __interrupt\n"
	".text\n"
);

// Memory layout.
extern const char __data_start_rom[0];
extern char __data_start__[0];
extern char __data_end__[0];
extern char __bss_start__[0];
extern char __bss_end__[0];
extern char __heap_start__[0];
extern char __heap_end__[0];

#define STACK_CHECK_VALUE 0xDACABE06
extern char __stack_start__[0];

// Management business.
static volatile int * __nrf51event__;

#define NREG(ev) (*((volatile int *) (ev)))
#define NEVENT(ev) { __nrf51event__ = &NREG(ev); *__nrf51event__ = 0; }
#define NWAIT { while (!*__nrf51event__); *__nrf51event__ = 0; }


int _write(int fd, const char * text, int len) {
	int index = 0;
	while (index < len) {
		NEVENT(0x4000211C); // TXDRDY (used to confirm transmission finished)
		// TXD
		NREG(0x4000251C) = text[index];
		NWAIT;
		index++;
	}
	return len;
}

void __libc_init_array();

static const char * appname = "sled";

void __prestart() {
	// Set stackcheck
	NREG(__stack_start__) = STACK_CHECK_VALUE;

	// grr, this wasn't in docs I think - make this output
	NREG(0x50000518) = 1 << NRF51_UART_TX_PIN; // DIRSET
	// configure UART pin
	NREG(0x4000250C) = NRF51_UART_TX_PIN; // PSELTXD
	NREG(0x40002524) = 0x01D7E000; // BAUDRATE - 112500
	NREG(0x40002500) = 4; // ENABLE
	NREG(0x40002008) = 1; // STARTTX

	_write(1, "1\n", 2);

	// start LFCLK on Synth (we need it for the RTC)
	NEVENT(0x40000104); // LFCLKSTARTED
	NREG(0x40000518) = 2; // LFCLKSRC
	NREG(0x40000008) = 1; // LFCLKSTART
	NWAIT;

	// start RTC
	NREG(0x4000B000) = 1;

	_write(1, "2\n", 2);

	// Initialize memory

	memcpy(__data_start__, __data_start_rom, __data_end__ - __data_start__);

	_write(1, "3\n", 2);

	memset(__bss_start__, 0, __bss_end__ - __bss_start__);

	_write(1, "4\n", 2);

	// NOTE: We do not use _start because it messes with the stack pointer.
	// Instead imitate the ONE part of _start that actually matters.
	// Then fingers go firmly into ears.
	__libc_init_array();

	_write(1, "5\n", 2);

	printf("printf is now available\n");

	sled_main(1, (char **) &appname);

	printf("goodbye\n");

	while (1)
		_write(1, "E\n", 2);
}

// --- SECTION 2 - WILL THE LIBC SHUT UP ABOUT MAIN ---

int main(int argc, char ** argv) {
	// we don't use _start, see above call to __libc_init_array
	return 0;
}

// --- SECTION 3 - TIME ---

// Ok, some actual honest-to-goodness DRIVER CODE!
// Keep in mind all the threading and CPU context stuff's been written first.
// Most of the implementation, including the timebase maintenance, is in oscore_udate.
// At the maximum resolution, the RTC can run for 512 seconds without overflowing.
// So basically, every time anything calls udate, it resets the RTC.

// The actual value for this is ~30.517578125
// If you need too much precision you should probably just read the RTC directly;
//  you'd obviously be using it for SOMETHING driver-like...
#define NRF51_RTC_TICK_MICROSECONDS_I 30517
#define NRF51_RTC_TICK_MICROSECONDS_D  1000

static oscore_time osc_current_base_time;

// When we occasionally CLEAR the RTC to prevent overflow,
//  the CLEAR task takes quite some time.
// (see subsection 19.1.8 of nRF51_RM_v3.0.1)
// This flag is used to indicate any overflow condition is because of an unfinished CLEAR.
// It also contains the last amount of time read before the clear completed,
//  which may help reduce drift.
static oscore_time osc_clearing_time;

oscore_time oscore_udate(void) {
	// get RTC value
	int rtcVal = NREG(0x4000B504);
	oscore_time valUs = (((oscore_time) rtcVal) * NRF51_RTC_TICK_MICROSECONDS_I) / NRF51_RTC_TICK_MICROSECONDS_D;
	// only do the reset thing if above a certain value to reduce drift
	// (and prevent it endlessly being reset too fast)
	if (rtcVal < 0x10000) {
		// General runtime (not / no longer clearing)
		if (osc_clearing_time) {
			osc_current_base_time += osc_clearing_time;
			osc_clearing_time = 0;
		}
	} else {
		// Clearing
		if (!osc_clearing_time)
			NREG(0x4000B008) = 1; // CLEAR
		osc_clearing_time = valUs;
	}
	return osc_current_base_time + valUs;
}

int usleep(useconds_t time) {
	oscore_time desired_usec = oscore_udate() + time;
	while (oscore_udate() < desired_usec)
		oscore_task_yield();
	return 0;
}

// --- SECTION 4 - SINGLETHREADED OSCORE COMPONENTS ---

oscore_event oscore_event_new(void) {
	char * ev = malloc(1);
	if (ev)
		*ev = 0;
	return (oscore_event) ev;
}

int oscore_event_wait_until(oscore_event ev, oscore_time desired_usec) {
	oscore_time tnow = oscore_udate();
	if (tnow >= desired_usec)
		return tnow;
	while (oscore_udate() < desired_usec) {
		if (*((char *) ev)) {
			*((char *) ev) = 0;
			return 1;
		}
		oscore_task_yield();
	}
	return 0;
}

void oscore_event_signal(oscore_event ev) {
	*((char *) ev) = 1;
}

void oscore_event_free(oscore_event ev) {
	free(ev);
}

int oscore_ncpus(void) {
	return 1;
}

void oscore_task_setprio(oscore_task task, int prio) {}
void oscore_task_pin(oscore_task task, int cpu) {}

// -- mutex
// before we begin, keep in mind that this oscore backend uses
//  a custom cooperative multitasking kernel, on a CPU that has only one core.
// we really, really, couldn't care less about atomics.
oscore_mutex oscore_mutex_new(void) {
	char * mutex = malloc(1);
	if (mutex)
		*mutex = 0;
	return mutex;
}

void oscore_mutex_lock(oscore_mutex m) {
	char * mutex = (char *) m;
	while (*mutex)
		oscore_task_yield();
	*mutex = 1;
}

void oscore_mutex_unlock(oscore_mutex m) {
	char * mutex = (char *) m;
	*mutex = 0;
}

void oscore_mutex_free(oscore_mutex m) {
	free(m);
}

// --- SECTION 5 - THE HANDHELD PORTAL DEVICE ---

typedef struct {
	// THIS FORM IS RELIED UPON FROM ASSEMBLY
	// The current SP (r13) value.
	void * sp;
	// The current PC (r15) value.
	void * pc;
} oscore_task_context_t;

// this exists just to get the compiler to know this symbol exists
extern void __oscore_task_portal_fling_switchback();
extern void __oscore_task_portal_open();

#define THUMB_PTR(n) ((void *) (((long) (n)) | 1))

__asm__(
	"__oscore_task_portal_open2:\n"
	// r15 is, of course, the target PC (INCLUDING THUMB FLAG)
	// for how this stack is setup, see oscore_task_create
	"pop {r15}\n"
	"__oscore_task_portal_open:\n"
	"bl __oscore_task_portal_open2\n"
	// we need a crashpad for tasks *anyway* so let's just make it the infinite loop we need to have regardless
	"__oscore_task_portal_done:\n"
	"bl oscore_task_yield\n"
	"b __oscore_task_portal_done\n"
);

// This switches context from saveTo to loadFrom.
static void oscore_task_portal_fling(oscore_task_context_t * saveTo, oscore_task_context_t * loadFrom) {
	// Before we begin, set PC so the ASM code doesn't have to
	saveTo->pc = THUMB_PTR(__oscore_task_portal_fling_switchback);
	// the following code relies pretty heavily on the compiler understanding the meaning of 'clobber'.
	__asm__(
		// capture inputs in clobbers {
		"mov r8, %0\n"
		"mov r9, %1\n"
		"mov r10, %2\n"
		// }
		// save context {
		"push {r0, r1, r2, r3, r4, r5, r6, r7}\n" // save set I
		// }
		// SP save {
		"mov r1, r8\n" // address into lo
		"mov r0, r13\n"
		"str r0, [r1, #0]\n"
		// }
		// ALL REGISTERS EXCEPT SP AND PC GET CLOBBERED {
		"mov r13, r9\n" // set SP
		"mov r15, r10\n" // set PC (this completes the context switch!)
		"__oscore_task_portal_fling_switchback:\n"
		// }
		// load context {
		"pop {r0, r1, r2, r3, r4, r5, r6, r7}\n" // load set I
		// }
		:
		: "r"(saveTo), "r"(loadFrom->sp), "r"(loadFrom->pc)
		// Try to hand off responsibility for all the difficult registers to the compiler.
		// R13 is SP, so it's saved, and R15 is PC, so it's also saved.
		: "r8", "r9", "r10", "r11", "r12", "r14", "memory"
	);
}

// --- SECTION 6 - PORTAL LINKAGE PAIRS ---

#define THREAD_STACK_SIZE_WORDS 0x100
typedef struct {
	oscore_task_context_t ctx;
	// These are NOT a ring, despite the round-robin structure;
	// this is so that osc_main_thread is immediately valid.
	// Thus, NULL indicates end-of-the-line.
	void * prev;
	void * next;
	// If NULL, the stack is owned elsewhere
	void ** stack;
	// At the very start of stack memory...
	int * stackcheck;
	// -- oscore api stuff --
	// Indicates deadness.
	int dead;
	// Function to run (see oscore_task_kick)
	oscore_task_function fn;
	// When alive, points to the passed 'ctx'. When dead, points to the result.
	void * userdata;
} oscore_task_t;

static oscore_task_t osc_main_thread = {
	.stackcheck = (int *) __stack_start__
};
// data (also, this is thread-local)
static oscore_task_t * osc_current_task = &osc_main_thread;

// The start function that __oscore_task_portal_open calls.
void oscore_task_kick() {
	osc_current_task->userdata = osc_current_task->fn(osc_current_task->userdata);
	osc_current_task->dead = 1;
}

oscore_task oscore_task_create(const char* name, oscore_task_function func, void* ctx) {
	oscore_task_t * tsk = malloc(sizeof(oscore_task_t));
	if (!tsk)
		return NULL;

	// setup API stuff
	tsk->dead = 0;
	tsk->fn = func;
	tsk->userdata = ctx;

	// setup the task's prev/next
	tsk->prev = osc_current_task;
	tsk->next = osc_current_task->next;

	// fixup inward connections
	if (osc_current_task->next)
		((oscore_task_t *) (osc_current_task->next))->prev = tsk;
	osc_current_task->next = tsk;

	// details of this function are important to what is about to happen to stack
	tsk->ctx.pc = THUMB_PTR(__oscore_task_portal_open);
	tsk->stack = malloc(sizeof(void *) * THREAD_STACK_SIZE_WORDS);
	tsk->stackcheck = (int *) tsk->stack;
	tsk->stackcheck[0] = STACK_CHECK_VALUE;
	// in memory order, which is the order used for push/pop
	tsk->stack[THREAD_STACK_SIZE_WORDS - 1] = THUMB_PTR(oscore_task_kick);
	tsk->ctx.sp = tsk->stack + THREAD_STACK_SIZE_WORDS - 1;
	return NULL;
}

void oscore_task_yield(void) {
	// sanity check
	assert(osc_current_task->stackcheck[0] == STACK_CHECK_VALUE);
	// So before we begin, keep in mind that everything's been aligned so that we already start in a valid state,
	//  from the very first instruction the CPU executes.
	// This is because SP is set in the vector table to point into osc_main_thread.
	oscore_task_t * oldT = osc_current_task;
	oscore_task_t * newT = osc_current_task->next;
	if (!newT) {
		newT = oldT;
		while (newT->prev)
			newT = newT->prev;
	}
	if (oldT != newT) {
		osc_current_task = newT;
		oscore_task_portal_fling(&oldT->ctx, &newT->ctx);
	}
}

void oscore_task_exit(void * status) {
	// bye!
	osc_current_task->dead = 1;
	osc_current_task->userdata = status;
	while (1)
		oscore_task_yield();
}

void * oscore_task_join(oscore_task task) {
	// notably, this function relies very heavily on the fact you can never get a pointer to the main task.
	// if you could, you'd be able to free() a pointer to a non-heap block.
	oscore_task_t * tsk = (oscore_task_t *) task;
	while (!tsk->dead)
		oscore_task_yield();
	void * status = tsk->userdata;
	// okie-dokie-lokie, the task is dead
	// unlink
	if (tsk->prev)
		((oscore_task_t *) tsk->prev)->next = tsk->next;
	if (tsk->next)
		((oscore_task_t *) tsk->next)->prev = tsk->prev;
	// YOU are the weakest task, tsk! *goodbye.*
	if (tsk->stack)
		free(tsk->stack);
	free(tsk);
	return status;
}

// --- SECTION X - SYSCALL IMPLEMENTATIONS (WHERE NOT _write) ---

// NOTE: as a .data variable, this requires setup; this is __prestart's job
static char * osc_brk = __heap_start__;

static int osc_already_exiting = 0;
void _exit(int err) {
	if (osc_already_exiting)
		while (1) {}
	osc_already_exiting = 1;
	assert(0);
}

int _read(int fd, char * data, int len) {
	errno = ENOSYS;
	return -1;
}

int _lseek(int fd, int offset, int whence) {
	errno = ENOSYS;
	return -1;
}

int _close() {
	// can't really happen since we can never open an FD, & stderr/stdout/stdin are off-limits
	// so use the same trick as _getpid just to shave some bytes
	return 0;
}

void * _sbrk(int move) {
	void * old = (void *) osc_brk;
	osc_brk += move;
	if (osc_brk > __heap_end__) {
		osc_brk -= move;
		errno = ENOMEM;
		return (void *) -1;
	}
	return old;
}

int _getpid() {
	// we are the init, we are all the init
	// (byte-for-byte, it's less code to emulate this correctly than it is to return an error)
	return 1;
}

int _kill(int pid, int sig) {
	errno = ENOSYS;
	return -1;
}

int _fstat(void * a, void * b) {
	errno = ENOSYS;
	return -1;
}

int _isatty(int file) {
	return 1;
}


