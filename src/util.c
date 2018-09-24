// Random utilities

#include "types.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

size_t util_strlcpy(char* dst, char* src, size_t bufsz) {
	strncpy(dst, src, bufsz);
	dst[bufsz - 1] = 0;
	return strlen(src);
}

int util_parse_int(char* str) {
	return (int) strtol(str, (char* *) NULL, 10);
}
