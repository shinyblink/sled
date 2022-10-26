#ifndef __INCLUDED_PRINTBUFFER__
#define __INCLUDED_PRINTBUFFER__
#include <types.h>

void printbuffer_clear(int from, int to, RGB fg, RGB bg);

void printbuffer_reset(void);

void printbuffer_init(int row, int column, RGB fg, RGB bg);

void printbuffer_init_default(void);

void printbuffer_deinit(void);

#define printbuffer_flag_blink (1)
#define printbuffer_flag_altchar (1 << 1)
// (1 << 2) is hidden

// if last column is reached automatic \n\r is done
void printbuffer_write(const char *str, int *row, int *column, RGB fg, RGB bg,
                       int flags);
void printbuffer_write_default(const char *str, int *row, int *column);

// this calls matrix_set
// blink delay tells how many draws it takes to toggle blinking once
// bits is from an .xbm file
// xbm must have 16 characters per row
// xbm must contain 128 characters
void printbuffer_draw(unsigned char bits[], int font_width, int font_height,
                      int blink_delay);
void printbuffer_draw_default(void);
#endif
