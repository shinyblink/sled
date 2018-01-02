// Random utilities

#include <types.h>
#include <stddef.h>
#include <string.h>

size_t util_strlcpy(char* dst, char* src, size_t bufsz) {
	strlcpy(dst, src, bufsz);
	dst[bufsz - 1] = 0;
	return strlen(src);
}
