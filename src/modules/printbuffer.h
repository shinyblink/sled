#ifndef __INCLUDED_PRINTBUFFER__
#define __INCLUDED_PRINTBUFFER__
#include <types.h>

void printbuffer_clear(int from, int to, RGB fg, RGB bg);

void printbuffer_clear_default();

void printbuffer_init(int row, int column, RGB fg, RGB bg);

void printbuffer_init_default();

void printbuffer_deinit();


void printbuffer_write(char *str, int *row, int *column, RGB fg, RGB bg);
void printbuffer_write_default(char *str, int *row, int *column);
// c is the character you want to load
// x and y are pixel inside the character
// xbm must have 16 characters per row
// xbm must contain 128 characters
int load_xbm_char(unsigned char bits[], char c, int x, int y, int w, int h);


// this calls matrix_set
void printbuffer_draw(unsigned char bits[], int font_width, int font_height);
void printbuffer_draw_default();
#endif