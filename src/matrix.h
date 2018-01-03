// Helper definitions that will allow one to manipulate the matrix.

#include <types.h>

extern int matrix_init(int outmodno);
extern int matrix_getx(void);
extern int matrix_gety(void);
extern int matrix_set(int x, int y, RGB *color);
extern int matrix_fill(int start_x, int start_y, int end_x, int end_y, RGB *color);
extern int matrix_clear(void);
extern int matrix_render(void);
extern int matrix_deinit(void);
