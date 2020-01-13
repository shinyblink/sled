#ifndef __INCLUDED_PRINTBUFFER__
#define __INCLUDED_PRINTBUFFER__
#include <types.h>

void printbuffer_clear(int from, int to, RGB fg, RGB bg);

void printbuffer_clear_default();

void printbuffer_init(int row, int column, RGB fg, RGB bg);

void printbuffer_init_default();

void printbuffer_deinit();

#define printbuffer_flag_blink 1
#define printbuffer_flag_altchar (1<<1)


void printbuffer_write(const char *str, int *row, int *column, RGB fg, RGB bg, int flags);
void printbuffer_write_default(const char *str, int *row, int *column);
// c is the character you want to load
// x and y are pixel inside the character
// xbm must have 16 characters per row
// xbm must contain 128 + 128 characters
// the second 128 characters get accessed with flag_altchar
int load_xbm_char(unsigned char bits[], unsigned char c, int x, int y, int w, int h, int flags);


// this calls matrix_set
// blink delay tells how many draws it takes to toggle blinking once
void printbuffer_draw(unsigned char bits[], int font_width, int font_height, int blink_delay);
void printbuffer_draw_default();
#endif