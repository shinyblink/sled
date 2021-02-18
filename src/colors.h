#ifndef __COLORS_H__
#define __COLORS_H__

#include <types.h>
#include <stdint.h>

#if !defined(RGB565_ORDER_RGB) && !defined(RGB565_ORDER_BGR)
#define RGB565_ORDER_RGB
#endif

static inline uint16_t RGB2RGB565(RGB color) {
	byte b = (color.blue  * 249 + 1014) >> 11;
	byte g = (color.green * 243 +  505) >> 10;
	byte r = (color.red   * 249 + 1014) >> 11;

#if defined(RGB565_ORDER_RGB)
	return (uint16_t) ((r << 11) | (g << 5) | b);
#elif defined(RGB565_ORDER_BGR)
	return (uint16_t) ((b << 11) | (g << 5) | r);
#endif
}

static inline RGB RGB5652RGB(uint16_t color) {
	uint8_t r5 = (color >> 11) & 0x1F;
	uint8_t g6 = (color >> 5) & 0x3F;
	uint8_t b5 = (color & 0x1F);

	uint8_t r = ((r5 * 527) + 23) >> 6;
	uint8_t g = ((g6 * 259) + 33) >> 6;
	uint8_t b = ((b5 * 527) + 23) >> 6;

#if defined(RGB565_ORDER_RGB)
	return RGB(r, g, b);
#elif defined(RGB565_ORDER_BGR)
	return RGB(b, g, r);
#endif
}

#endif
