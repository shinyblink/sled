#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define  MXC_ASSERT_ENABLE
#include "mxc_assert.h"

#include "max32665.h"

/* CMSIS keeps a global updated with current system clock in Hz */
#define configCPU_CLOCK_HZ          ((unsigned long)96000000)

/* TODO: Adjust this for tickless idle */
#define configTICK_RATE_HZ          ((portTickType)1000)

/* Memory
 *
 * Heap is managed by libc (heap_3.c).
 */
#define configMINIMAL_STACK_SIZE    ((unsigned short)256)

/* FIXME: Assign proper priorities to all interrupts */
#define configMAX_PRIORITIES        5
/* # of priority bits (configured in hardware) is provided by CMSIS */
#define configPRIO_BITS             __NVIC_PRIO_BITS
/* Priority 7, or 255 as only the top three bits are implemented.  This is the lowest priority. */
#define configKERNEL_INTERRUPT_PRIORITY \
	( ( unsigned char ) 7 << ( 8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
	( ( unsigned char ) 2 << ( 8 - configPRIO_BITS) )

/* We want to use preemption to easier integrate components */
#define configUSE_PREEMPTION        1

/*
 * Tickless idle from the FreeRTOS port + our own hooks (defined further down in
 * this file)
 */
#define configUSE_TICKLESS_IDLE     1

/* TODO: Adjust */
#define configUSE_IDLE_HOOK         0
#define configUSE_TICK_HOOK         0
#define configUSE_CO_ROUTINES       0
#define configUSE_16_BIT_TICKS      0
#define configUSE_MUTEXES           1
#define configUSE_TIMERS            1

#define configTIMER_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH    10
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2)

#define INCLUDE_vTaskSuspend        1
#define INCLUDE_vTaskDelay          1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_vTaskDelete			1
//#define INCLUDE_xTaskCreatePinnedToCore		1
#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_vTaskStartScheduler			1
#define INCLUDE_vTaskSwitchContext			1
#define INCLUDE_xTimerCreateTimerTask		1
//#define INCLUDE_vPortSuppressTicksAndSleep	1

/* Allow static allocation of data structures */
#define configSUPPORT_STATIC_ALLOCATION 0

/*
 * Enable stack overflow detector.
 *
 * TODO: Remove for production.
 */
#define configCHECK_FOR_STACK_OVERFLOW 0

/* Alias the default handler names to match CMSIS weak symbols */
#define vPortSVCHandler       SVC_Handler
#define xPortPendSVHandler    PendSV_Handler
#define xPortSysTickHandler   SysTick_Handler

/* Assert */
#define configASSERT(x)       MXC_ASSERT(x)

/* Tickless idle hooks */
typedef uint32_t TickType_t;
void pre_idle_sleep(TickType_t xExpectedIdleTime);
#define configPRE_SLEEP_PROCESSING(xModifiableIdleTime) \
	pre_idle_sleep(xModifiableIdleTime); xModifiableIdleTime = 0

void post_idle_sleep(TickType_t xExpectedIdleTime);
#define configPOST_SLEEP_PROCESSING(xExpectedIdleTime) \
	post_idle_sleep(xExpectedIdleTime)

/*
 * Uncomment to trace FreeRTOS malloc wrapper.
 *
 */
// extern int printf (const char *__restrict __format, ...);
// #define traceMALLOC( pvAddress, uiSize ) printf("[%s:%d] %p %d\n", __FILE__, __LINE__, pvAddress, uiSize)

#endif /* FREERTOS_CONFIG_H */
