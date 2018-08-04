#include "types.h"

extern int graphics_drawline_core(int x0, int y0, int x1, int y1, int (*set)(int, int, void*), void *ud);
extern int graphics_drawline_matrix(int x, int y, const RGB* col);
#define graphics_drawline(x0, y0, x1, y1, colour) graphics_drawline_core(x0, y0, x1, y1, (int (*)(int, int, void*)) graphics_drawline_matrix, &colour)

extern int graphics_drawcircle(int x0, int y0, byte radius, RGB color);
