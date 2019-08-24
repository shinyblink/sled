#ifndef __COLORS_H__
#define __COLORS_H__

#include <types.h>
#include <stdint.h>

static inline uint16_t RGB2RGB565(RGB col) {
	uint16_t b = (col.blue >> 3) & 0x1F;
	uint16_t g = ((col.green >> 2) & 0x3F) << 5;
	uint16_t r = ((col.red >> 3) & 0x1F) << 11;

	return (uint16_t) (r | g | b);
}

static inline RGB RGB5652RGB(uint16_t color) {
	uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
	uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
	uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;

	return RGB(r, g, b);
}

#endif
