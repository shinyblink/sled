// "graphics", some helpers for matrix stuff.

#include <types.h>
#include <matrix.h>
#include <math.h>
#include <mathey.h>
#include <stdlib.h>

// TODO: replace this. 20kdc, looking at you.
void graphics_drawline(byte x1, byte y1, byte x2, byte y2, RGB *color)
{
  int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

  dx =x2 - x1;      /* the horizontal distance of the line */
  dy =y2 - y1;      /* the vertical distance of the line */
  dxabs = abs(dx);
  dyabs = abs(dy);
  sdx = sign(dx);
  sdy = sign(dy);
  x = dyabs >> 1;
  y = dxabs >> 1;
  px = x1;
  py = y1;

  matrix_set(px, py, color);

  if (dxabs >= dyabs) { /* the line is more horizontal than vertical */
    for(i = 0; i < dxabs; ++i) {
      y += dyabs;
      if (y >= dxabs) {
        y -= dxabs;
        py += sdy;
      }
      px += sdx;
      matrix_set(px, py, color);
    }
  } else { // the line is more vertical than horizontal
    for(i = 0;i < dyabs; ++i) {
      x += dxabs;
      if (x >= dyabs) {
        x -= dyabs;
        px += sdx;
      }
      py += sdy;
      matrix_set(px, py, color);
    }
  }
}
