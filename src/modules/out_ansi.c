// ANSI terminal output.
// You're gonna need unicode support,
// 24-bit color support and a whole
// lot of luck for this.
// TODO: support lesser terminals?
// xst rules, but many people use
// a lot different terminals.

#include <types.h>
#include <timers.h>
#include <assert.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <taskpool.h>

// not sure which i like more..
#define TERM_CHAR "●"
//#define TERM_CHAR "o"

// Escapes.
#define ESC "\x1B["
#define MOVETO "%i;%iH"
#define HIDECURSOR "?25l"
#define SHOWCURSOR "?25h"
#define TRUECOLOR "38;2;%i;%i;%im"
#define NOCOLOR "0m"

static int term_w, term_h;
static RGB* term_buf;

#define PPOS(x, y) (x + (y * term_w))

int init(int modno, char* argstr) {
	if (argstr)
		free(argstr);

	struct winsize winsz;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz);
	term_w = winsz.ws_col;
	term_h = winsz.ws_row - 1; // last two rows for status messages.

	if (!(term_w && term_h)) {
		eprintf("Not a terminal, stupid!\n");
		return 1;
	}

	term_buf = calloc(term_h, term_w * sizeof(RGB));
	if (!term_buf)
		return 1;
	printf(ESC HIDECURSOR);
	return 0;
}

int getx(void) {
	return term_w;
}
int gety(void) {
	return term_h;
}

int set(int x, int y, RGB color) {
	term_buf[PPOS(x, y)] = color;
	return 0;
}

RGB get(int x, int y) {
	return term_buf[PPOS(x, y)];
}

int clear(void) {
	memset(term_buf, 0, term_w * term_h * sizeof(RGB));
	return 0;
};

int render(void) {
	int x, y;
	fprintf(stdout, ESC MOVETO, 0, 0);
	for (y = 0; y < term_h; y++) {
		for (x = 0; x < term_w; x++) {
			RGB px = term_buf[PPOS(x, y)];
			fprintf(stdout, ESC TRUECOLOR TERM_CHAR, px.red, px.green, px.blue);
		}
	}
	fprintf(stdout, ESC MOVETO ESC NOCOLOR, term_h + 1, 0);
	fflush(stdout);
	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return wait_until_core(desired_usec);
}

void wait_until_break(void) {
	wait_until_break_core();
}

int deinit(void) {
	printf(ESC "2J" ESC "H" ESC SHOWCURSOR);
	fflush(stdout);
	return 0;
}
