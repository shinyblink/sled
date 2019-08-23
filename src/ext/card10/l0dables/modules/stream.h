#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>

#ifndef __SPHINX_DOC
/* stddef.h is not recognized by hawkmoth for some odd reason */
#include <stddef.h>
#else
typedef unsigned int size_t;
#endif /* __SPHINX_DOC */

#include "FreeRTOS.h"
#include "queue.h"

/* Time to wait for the descriptor table lock to become available */
#define STREAM_MUTEX_WAIT pdMS_TO_TICKS(100)

/**
 * **Stream Descriptors**:
 *
 *    This enum defines all known stream descriptors.  Internally, the stream
 *    module allocates an array slot for each ID defined here.  For that to
 *    work, :c:macro:`SD_MAX` must be greater than the highest defined ID.
 *    Please keep IDs in sequential order.
 */
enum stream_descriptor {
	/** Highest descriptor must always be ``SD_MAX``. */
	SD_MAX,
};

/**
 * Stream Information Object.
 *
 * This struct contains the information necessary for :c:func:`epic_stream_read`
 * to read from a sensor's stream.  This consists of:
 */
struct stream_info {
	/**
	 * A FreeRTOS queue handle.
	 *
	 * Management of this queue is the sensor drivers responsibility.
	 */
	QueueHandle_t queue;
	/** The size of one data packet (= queue element). */
	size_t item_size;
	/**
	 * An optional function to call before performing the read.  Set to
	 * ``NULL`` if unused.
	 *
	 * ``poll_stream()`` is intended for sensors who passively collect data.
	 * A sensor driver might for example retrieve the latest samples in this
	 * function instead of actively polling in a task loop.
	 *
	 * The function registered here should never block for a longer time.
	 */
	int (*poll_stream)();
};

/**
 * Register a stream so it can be read from Epicardium API.
 *
 * :param int sd: Stream Descriptor.  Must be from the :c:type:`stream_descriptor` enum.
 * :param stream_info stream: Stream info.
 * :returns: ``0`` on success or a negative value on error.  Possible errors:
 *
 *    - ``-EINVAL``: Out of range sensor descriptor.
 *    - ``-EACCES``: Stream already registered.
 *    - ``-EBUSY``: The descriptor lock could not be acquired.
 */
int stream_register(int sd, struct stream_info *stream);

/**
 * Deregister a stream.
 *
 * :param int sd:  Stream Descriptor.
 * :param stream_info stream: The stream which should be registered for the
 *    stream ``sd``.  If a different stream is registered, this function
 *    will return an error.
 * :returns: ``0`` on success or a negative value on error.  Possible errors:
 *
 *    - ``-EINVAL``: Out of range sensor descriptor.
 *    - ``-EACCES``: Stream ``stream`` was not registered for ``sd``.
 *    - ``-EBUSY``: The descriptor lock could not be acquired.
 */
int stream_deregister(int sd, struct stream_info *stream);

/*
 * Initialize stream interface.  Called by main().
 */
int stream_init();

#endif /* STREAM_H */
