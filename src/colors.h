#ifndef __COLORS_H__
#define __COLORS_H__

#include <stdint.h>

static inline uint16_t RGB2RGB565(RGB col) {
	uint16_t b = (col.blue >> 3) & 0x1F;
	uint16_t g = ((col.green >> 2) & 0x3F) << 5;
	uint16_t r = ((col.red >> 3) & 0x1F) << 11;

	return (uint16_t) (r | g | b);
}

#endif
