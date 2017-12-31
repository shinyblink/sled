// Helper definitions that will allow one to manipulate the matrix.

#include <types.h>

extern int matrix_init(void);
extern int matrix_ppos(byte x, byte y);
extern int matrix_set(byte x, byte y, RGB *color);
extern int matrix_fill(byte start_x, byte start_y, byte end_x, byte end_y, RGB *color);
extern int matrix_clear(void);
extern int matrix_render(void);
extern int matrix_deinit(void);
