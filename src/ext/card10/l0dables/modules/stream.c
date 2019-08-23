#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "epicardium.h"
#include "modules/log.h"
#include "modules/stream.h"

/* Internal buffer of registered streams */
static struct stream_info *stream_table[SD_MAX];

/* Lock for modifying the stream info table */
static StaticSemaphore_t stream_table_lock_data;
static SemaphoreHandle_t stream_table_lock;

int stream_init()
{
	memset(stream_table, 0x00, sizeof(stream_table));
	stream_table_lock =
		xSemaphoreCreateMutexStatic(&stream_table_lock_data);
	return 0;
}

int stream_register(int sd, struct stream_info *stream)
{
	if (xSemaphoreTake(stream_table_lock, STREAM_MUTEX_WAIT) != pdTRUE) {
		LOG_WARN("stream", "Lock contention error");
		return -EBUSY;
	}

	if (sd < 0 || sd >= SD_MAX) {
		return -EINVAL;
	}

	if (stream_table[sd] != NULL) {
		/* Stream already registered */
		return -EACCES;
	}

	stream_table[sd] = stream;

	xSemaphoreGive(stream_table_lock);
	return 0;
}

int stream_deregister(int sd, struct stream_info *stream)
{
	if (xSemaphoreTake(stream_table_lock, STREAM_MUTEX_WAIT) != pdTRUE) {
		LOG_WARN("stream", "Lock contention error");
		return -EBUSY;
	}

	if (sd < 0 || sd >= SD_MAX) {
		return -EINVAL;
	}

	if (stream_table[sd] != stream) {
		/* Stream registered by someone else */
		return -EACCES;
	}

	stream_table[sd] = NULL;

	xSemaphoreGive(stream_table_lock);
	return 0;
}

int epic_stream_read(int sd, void *buf, size_t count)
{
	/*
	 * TODO: In theory, multiple reads on different streams can happen
	 * simulaneously.  I don't know what the most efficient implementation
	 * of this would look like.
	 */
	if (xSemaphoreTake(stream_table_lock, STREAM_MUTEX_WAIT) != pdTRUE) {
		LOG_WARN("stream", "Lock contention error");
		return -EBUSY;
	}

	if (sd < 0 || sd >= SD_MAX) {
		return -EBADF;
	}

	struct stream_info *stream = stream_table[sd];
	if (stream == NULL) {
		return -ENODEV;
	}

	/* Poll the stream, if a poll_stream function exists */
	if (stream->poll_stream != NULL) {
		int ret = stream->poll_stream();
		if (ret < 0) {
			return ret;
		}
	}

	/* Check buffer size is a multiple of the data packet size */
	if (count % stream->item_size != 0) {
		return -EINVAL;
	}

	size_t i;
	for (i = 0; i < count; i += stream->item_size) {
		if (!xQueueReceive(stream->queue, buf + i, 0)) {
			break;
		}
	}

	xSemaphoreGive(stream_table_lock);
	return i / stream->item_size;
}
