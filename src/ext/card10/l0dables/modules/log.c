#include "modules/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdarg.h>

#if LOG_ENABLE
int log_msg(const char *subsys, const char *format, ...)
{
	va_list ap;
	int ret;

	if (!LOG_ENABLE_DEBUG && format[0] == 'D') {
		return 0;
	}

	va_start(ap, format);
	if (LOG_ENABLE_COLOR) {
		char *msg_color = "";

		switch (format[0]) {
		case 'D':
			msg_color = "\x1B[2m";
			break;
		case 'W':
			msg_color = "\x1B[1m";
			break;
		case 'E':
			msg_color = "\x1B[31m";
			break;
		case 'C':
			msg_color = "\x1B[37;41;1m";
			break;
		}

		printf("\x1B[32m[%12lu] \x1B[33m%s\x1B[0m: %s",
		       xTaskGetTickCount(),
		       subsys,
		       msg_color);

		ret = vprintf(&format[1], ap);

		printf("\x1B[0m\n");
	} else {
		printf("[%12lu] %s: ", xTaskGetTickCount(), subsys);
		ret = vprintf(&format[1], ap);
		printf("\n");
	}
	va_end(ap);

	return ret;
}
#endif /* LOG_ENABLE */
