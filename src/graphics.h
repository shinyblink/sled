#include "types.h"

extern int graphics_drawline_core(int x0, int y0, int x1, int y1, int (*set)(int, int, void*), void *ud);
#define graphics_drawline(x0, y0, x1, y1, colour) graphics_drawline_core(x0, y0, x1, y1, (int (*)(int, int, void*)) matrix_set, colour)

extern int graphics_drawcircle(int x0, int y0, byte radius, RGB *color);
