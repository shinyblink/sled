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
static double t;

static double frand(double max)
{
  return max * rand() / RAND_MAX;
}

static void fadeby(double f)
{
  for (int y = 0; y < screenH; ++y)
  {
    for (int x = 0; x < screenW; ++x)
    {
      RGB col = matrix_get(x, y);
      col.red *= f;
      col.green *= f;
      col.blue *= f;
      matrix_set(x, y, col);
    }
  }
}

int init(int moduleid, char* argstr)
{
   screenW = matrix_getx();
   screenH = matrix_gety();
   return 0;
}

void reset(int moduleid)
{
  t = frand(1e6);
}

int draw(int moduleid, int argc, char* argv[])
{
  static double x1, y1;
  static int forever = 0;
  
  oscore_time now = udate();
  fadeby(0.98);

  double a = 4 * sin(t * 1.05);
  double b = 4 * sin(t * 1.07);
  double c = 4 * sin(t * 1.11);
  double d = 4 * sin(t * 1.13);
  
  if (argc)
  {
    forever = (argv[0][0] == 'f');
  }
  
  int over = 0;
  for (int i = 0; i < STEPS; i++)
  {
    double x2 = sin(a * y1) - cos(b * x1);
    double y2 = sin(c * x1) - cos(d * y1);
    double dx = x2 - x1;
    double dy = y2 - y1;
    int dr = (80000.0 / STEPS) * fabs(dx);
    int dg = (80000.0 / STEPS) * fabs(dy);
    int db = (80000.0 / STEPS);
    int x = x2 * 0.25 * (screenW - 11) + screenW * 0.5;
    int y = y2 * 0.25 * (screenH - 11) + screenH * 0.5;
    RGB col = matrix_get(x, y);
    col = RGB(MIN(col.red + dr, 255), MIN(col.green + dg, 255), MIN(col.blue + db, 255)); 
    if (col.blue == 255) over++;
    matrix_set(x, y, col);
    x1 = x2;
    y1 = y2;
  }
  double speed = (over - 0.3 * STEPS) / (0.015 * STEPS);
  speed = MAX(1.0, speed);
  t += 0.0005 * speed;

  matrix_render();
  if (frame++ >= FRAMES)
  {
    frame = 0;
    if (!forever) return 1;
  }
  oscore_time nexttick = now + T_SECOND / FPS;
  timer_add(nexttick, moduleid, 0, NULL);
  return 0;
}

void deinit(int moduleid)
{
}
