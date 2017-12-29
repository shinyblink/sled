// "graphics", some helpers for matrix stuff.

#include <types.h>
#include <matrix.h>
#include <math.h>
#include <mathey.h>
#include <stdlib.h>

// TODO: replace this. 20kdc, looking at you.
int graphics_drawline(byte x1, byte y1, byte x2, byte y2, RGB *color)
{
	int ret,i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

	dx =x2 - x1;	/* the horizontal distance of the line */
	dy =y2 - y1;	/* the vertical distance of the line */
	dxabs = abs(dx);
	dyabs = abs(dy);
	sdx = sign(dx);
	sdy = sign(dy);
	x = dyabs >> 1;
	y = dxabs >> 1;
	px = x1;
	py = y1;

	ret = matrix_set(px, py, color);
	if (ret != 0) return ret;

	if (dxabs >= dyabs) { /* the line is more horizontal than vertical */
		for(i = 0; i < dxabs; ++i) {
			y += dyabs;
			if (y >= dxabs) {
				y -= dxabs;
				py += sdy;
			}
			px += sdx;
			ret = matrix_set(px, py, color);
			if (ret != 0) return ret;
		}
	} else { // the line is more vertical than horizontal
		for(i = 0;i < dyabs; ++i) {
			x += dxabs;
		if (x >= dyabs) {
				x -= dyabs;
				px += sdx;
			}
			py += sdy;
			ret = matrix_set(px, py, color);
			if (ret != 0) return ret;
		}
	}
	return 0;
}

// stolen from wikipedia because idk.
void graphics_drawcircle(byte x0, byte y0, byte radius, RGB *color) {
	byte x = radius-1;
	byte y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (radius << 1);

	while (x >= y) {
		matrix_set(x0 + x, y0 + y, color);
		matrix_set(x0 + y, y0 + x, color);
		matrix_set(x0 - y, y0 + x, color);
		matrix_set(x0 - x, y0 + y, color);
		matrix_set(x0 - x, y0 - y, color);
		matrix_set(x0 - y, y0 - x, color);
		matrix_set(x0 + y, y0 - x, color);
		matrix_set(x0 + x, y0 - y, color);

		if (err <= 0) {
			y++;
			err += dy;
			dy += 2;
		}
		if (err > 0) {
			x--;
			dx += 2;
			err += dx - (radius << 1);
		}
	}
}
