#include "epicardium.h"
#include "api/interrupt-sender.h"
#include "modules/log.h"
#include "modules/modules.h"

#include "max32665.h"
#include "cdcacm.h"
#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdint.h>
#include <stdio.h>

/* Task ID for the serial handler */
TaskHandle_t serial_task_id = NULL;

/* The serial console in use (UART0) */
extern mxc_uart_regs_t *ConsoleUart;
/* Read queue, filled by both UART and CDCACM */
static QueueHandle_t read_queue;

/*
 * API-call to write a string.  Output goes to both CDCACM and UART
 */
void epic_uart_write_str(const char *str, intptr_t length)
{
	UART_Write(ConsoleUart, (uint8_t *)str, length);
	cdcacm_write((uint8_t *)str, length);
	ble_uart_write((uint8_t *)str, length);
}

/*
 * API-call to read a character from the queue.
 */
int epic_uart_read_char(void)
{
	char chr;
	if (xQueueReceive(read_queue, &chr, 0) == pdTRUE) {
		return (int)chr;
	}
	return (-1);
}

/*
 * API-call to read data from the queue.
 */
int epic_uart_read_str(char *buf, size_t cnt)
{
	size_t i = 0;

	for (i = 0; i < cnt; i++) {
		if (xQueueReceive(read_queue, &buf[i], 0) != pdTRUE) {
			break;
		}
	}

	return i;
}

long _write_epicardium(int fd, const char *buf, size_t cnt)
{
	/*
	 * Only print one line at a time.  Insert `\r` between lines so they are
	 * properly displayed on the serial console.
	 */
	size_t i, last = 0;
	for (i = 0; i < cnt; i++) {
		if (buf[i] == '\n') {
			epic_uart_write_str(&buf[last], i - last);
			epic_uart_write_str("\r", 1);
			last = i;
		}
	}
	epic_uart_write_str(&buf[last], cnt - last);
	return cnt;
}

/* Interrupt handler needed for SDK UART implementation */
void UART0_IRQHandler(void)
{
	UART_Handler(ConsoleUart);
}

static void uart_callback(uart_req_t *req, int error)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(serial_task_id, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void serial_enqueue_char(char chr)
{
	if (chr == 0x3) {
		/* Control-C */
		api_interrupt_trigger(EPIC_INT_CTRL_C);
	}

	if (xQueueSend(read_queue, &chr, 100) == errQUEUE_FULL) {
		/* Queue overran, wait a bit */
		vTaskDelay(portTICK_PERIOD_MS * 50);
	}

	api_interrupt_trigger(EPIC_INT_UART_RX);
}

void vSerialTask(void *pvParameters)
{
	static uint8_t buffer[sizeof(char) * SERIAL_READ_BUFFER_SIZE];
	static StaticQueue_t read_queue_data;

	serial_task_id = xTaskGetCurrentTaskHandle();

	/* Setup read queue */
	read_queue = xQueueCreateStatic(
		SERIAL_READ_BUFFER_SIZE, sizeof(char), buffer, &read_queue_data
	);

	/* Setup UART interrupt */
	NVIC_ClearPendingIRQ(UART0_IRQn);
	NVIC_DisableIRQ(UART0_IRQn);
	NVIC_SetPriority(UART0_IRQn, 6);
	NVIC_EnableIRQ(UART0_IRQn);

	unsigned char data;
	uart_req_t read_req = {
		.data     = &data,
		.len      = 1,
		.callback = uart_callback,
	};

	while (1) {
		int ret = UART_ReadAsync(ConsoleUart, &read_req);
		if (ret != E_NO_ERROR && ret != E_BUSY) {
			LOG_ERR("serial", "error reading uart: %d", ret);
			vTaskDelay(portMAX_DELAY);
		}

		ulTaskNotifyTake(pdTRUE, portTICK_PERIOD_MS * 1000);

		if (read_req.num > 0) {
			serial_enqueue_char(*read_req.data);
		}

		while (UART_NumReadAvail(ConsoleUart) > 0) {
			serial_enqueue_char(UART_ReadByte(ConsoleUart));
		}

		while (cdcacm_num_read_avail() > 0) {
			serial_enqueue_char(cdcacm_read());
		}
	}
}
