#pragma once

#define SEV_DEBUG "D"
#define SEV_INFO  "I"
#define SEV_WARN  "W"
#define SEV_ERR   "E"
#define SEV_CRIT  "C"

/* Whether to enable logging at all */
#ifndef LOG_ENABLE
#define LOG_ENABLE 1
#endif

/* Whether to enable even more verbose logging */
#ifndef LOG_ENABLE_DEBUG
#define LOG_ENABLE_DEBUG 0
#endif

/* Whether to enable colorful log */
#ifndef LOG_ENABLE_COLOR
#define LOG_ENABLE_COLOR 1
#endif

#if LOG_ENABLE

int log_msg(const char *subsys, const char *format, ...)
	__attribute__((format(printf, 2, 3)));

#if LOG_ENABLE_DEBUG
#define LOG_DEBUG(subsys, format, ...)                                         \
	log_msg(subsys, SEV_DEBUG format, ##__VA_ARGS__)
#else /* LOG_ENABLE_DEBUG */
#define LOG_DEBUG(subsys, format, ...)
#endif /* LOG_ENABLE_DEBUG */

#define LOG_INFO(subsys, format, ...)                                          \
	log_msg(subsys, SEV_INFO format, ##__VA_ARGS__)
#define LOG_WARN(subsys, format, ...)                                          \
	log_msg(subsys, SEV_WARN format, ##__VA_ARGS__)
#define LOG_ERR(subsys, format, ...)                                           \
	log_msg(subsys, SEV_ERR format, ##__VA_ARGS__)
#define LOG_CRIT(subsys, format, ...)                                          \
	log_msg(subsys, SEV_CRIT format, ##__VA_ARGS__)

#else /* LOG_ENABLE_DEBUG */

inline __attribute__((format(printf, 2, 3))) int
log_msg(const char *subsys, const char *format, ...)
{
	return 0;
}

#define LOG_DEBUG(subsys, format, ...)
#define LOG_INFO(subsys, format, ...)
#define LOG_WARN(subsys, format, ...)
#define LOG_ERR(subsys, format, ...)
#define LOG_CRIT(subsys, format, ...)

#endif /* LOG_ENABLE_DEBUG */
