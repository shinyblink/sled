// Rather simple rainbow.
// Some color blending function stolen from https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>

#define FRAMES 255
#define FRAMETIME ((RANDOM_TIME * T_SECOND) / 255)

static int modno;
static int pos;
static int frame;
static ulong nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	return 0;
}

int draw_row(int row, byte r, byte g, byte b) {
	RGB color = RGB(r, g, b);
	//return matrix_fill(row, 0, row, matrix_getx() - 1, &color);
	int y;
	for (y=0; y < matrix_gety(); ++y)
		matrix_set(row, y, &color);
	return 0;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void wheel(byte x, byte wheelpos) {
  wheelpos = 255 - wheelpos;

  if ( wheelpos < 85 ) {
    draw_row(x, 255 - wheelpos * 3, 0, wheelpos * 3);
  } else if( wheelpos < 170 ) {
    wheelpos -= 85;
    draw_row(x, 0, wheelpos * 3, 255 - wheelpos * 3);
  } else {
    wheelpos -= 170;
    draw_row(x, wheelpos * 3, 255 - wheelpos * 3, 0);
  }
}

int draw(int argc, char* argv[]) {
	if (frame == 0)
		nexttick = udate();

	int x;
	for (x = 0; x < matrix_getx(); ++x)
		wheel(x, pos + x);
	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		pos = 0;
		return 1;
	}
	frame++;
	pos++;;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int deinit() {
	return 0;
}
