// Rather simple sine drawing.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <math.h>
#include <stddef.h>
#include <graphics.h>
#include <stdlib.h>
#include <mathey.h>


#define FRAMETIME (T_SECOND / 30)
#define FRAMES (RANDOM_TIME * 30)
// Size of a full revolution
#define XSCALE (matrix_getx())
#define YSCALE (matrix_gety())

static int modno;
static int pos;
static int frame;
static ulong nexttick;

int init(int moduleno, char* argstr) {
	if (matrix_getx() < 3)
		return 1;
	modno = moduleno;
	frame = 0;
	return 0;
}

RGB white = { .red = 255, .green = 255, .blue = 255 };

void reset() {
	nexttick = udate();
	frame = 0;
}

int draw(int argc, char* argv[]) {
	vec2 p, _p;
	int dy;

	matrix_clear();

	for (p.x = -1; p.x <= XSCALE; p.x++) {
		p.y = - round((YSCALE - 1) / 2 * (sin((p.x + pos) * M_PI * 2.0f / (double) XSCALE) - 1));
		matrix_set(p.x, p.y, &white);
		vec2 d = vadd(_p, vmul(p, -1));
		dy = round(d.y);
		d = vmul(d, 1.0/d.y);
		for (int i = 1; abs(dy)>1, i<abs(dy); ++i)
			matrix_set((int) ((p.x==0&&dy>0)?_p.x:p.x + i*d.x), (int) (fmin(p.y,_p.y) + i*d.y), &white);
		memcpy(&_p, &p, sizeof(p));
	}

	matrix_render();

	if (frame >= FRAMES) {
		frame = 0;
		pos = 0;
		return 1;
	}

	frame++;
	pos++;
	pos = pos % XSCALE;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}

int deinit() {
	return 0;
}
