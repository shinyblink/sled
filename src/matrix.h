// Helper definitions that will allow one to manipulate the matrix.

#include <types.h>

extern int matrix_init(void);
extern int matrix_set(byte x, byte y, RGB *color);
extern int matrix_render(void);
extern int matrix_deinit(void);
