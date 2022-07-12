#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_LONG * FPS)

#define STEPS 20000

static int screenW;
static int screenH;
static int frame;
static double ax, ay;
static double a, b, c, d;
static double da, db, dc, dd;

static double frand(double max)
{
   return max * rand() / RAND_MAX;
}

int init(int moduleid, char* argstr)
{
   screenW = matrix_getx();
   screenH = matrix_gety();
   
   return 0;
}

void reset(int moduleid)
{
  a = frand(8) - 4;
  b = frand(8) - 4;
  c = frand(8) - 4;
  d = frand(8) - 4;
  da = a > 0 ? -0.001 : 0.001;
  db = b > 0 ? -0.001 : 0.001;
  dc = c > 0 ? -0.001 : 0.001;
  dd = d > 0 ? -0.001 : 0.001;
}

int draw(int moduleid, int argc, char* argv[])
{
  oscore_time now = udate();
  for (int y = 0; y < screenH; ++y)
  {
    for (int x = 0; x < screenW; ++x)
    {
      RGB col = matrix_get(x, y);
      col.red *= 0.98;
      col.green *= 0.98;
      col.blue *= 0.98;
      matrix_set(x, y, col);
    }
  }
  
  int over = 0;
  for (int i = 0; i < STEPS; i++)
  {
    double x2 = sin(a * ay) - cos(b * ax);
    double y2 = sin(c * ax) - cos(d * ay);
    int x = x2 * 0.23 * screenW + screenW * 0.5;
    int y = y2 * 0.23 * screenH + screenH * 0.5;
    double dx = ax - x2;
    double dy = ay - y2;
    int dr = 4 * fabs(dx);
    int dg = 4 * fabs(dy);
    int db = 4;
    RGB col = matrix_get(x, y);
    int red = col.red + dr;
    int green = col.green + dg;
    int blue = col.blue + db;
    if (red > 255) red = 255;
    if (green > 255) green = 255;
    if (blue > 255) {blue = 255; over++;}
    col.red =  red;
    col.green = green;
    col.blue = blue;
    matrix_set(x, y, col);
    ax = x2;
    ay = y2;
  }
  double speed = (double)(over - 5000) / 150;
  speed = speed < 1.0 ? 1.0 : speed;
  a += speed * da;
  b += speed * db;
  c += speed * dc;
  d += speed * dd;

  matrix_render();
  if (frame++ >= FRAMES)
  {
    frame = 0;
    return 1;
  }
  oscore_time nexttick = now + T_SECOND / FPS;
  timer_add(nexttick, moduleid, 0, NULL);
  return 0;
}

void deinit(int moduleid)
{
}
