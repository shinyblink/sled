#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <matrix.h>
#include <timers.h>

#define FPS 18
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (1.5 * TIME_LONG * FPS)

#define  V     12
#define  G      3
#define  SEEDS  6

static RGB cols[V + 1];

static uint8_t *cdat;
uint8_t *ndat;

static int screenW;
static int screenH;

int urand(int max)
{
   return rand() % max;
}

void sim(void)
{
   for (int y = 0; y < screenH; y++)
   {
      for (int x = 0; x < screenW; x++)
      {
         int sum = 0;
         int ill = 0;
         int infected = 0;
         int self = cdat[y * screenW + x];
         for (int dy = -1; dy <= 1; dy++)
         {
            for (int dx = -1; dx <= 1; dx++)
            {
               if (dx == 0 && dy == 0) continue;
               int mx = (x + dx + screenW) % screenW;
               int my = (y + dy + screenH) % screenH;
               int n = cdat[my * screenW + mx];
               sum += n;
               if (n >= V) ill++;
               else if (n > 0) infected++;
            }
         }
         if (self >= V)
         {
            ndat[y * screenW + x] = 0;
         }
         else if (self > 0)
         {
            ndat[y * screenW + x] = MIN(sum / 8 + G, V);
         }
         else
         {
            ndat[y * screenW + x] = infected + ill;
         }
      }
   }
   
   uint8_t *tmp = ndat;
   ndat = cdat;
   cdat = tmp;
}

int init(int moduleid, char* argstr)
{
   screenW = matrix_getx();
   screenH = matrix_gety();
   return 0;
}

void reset(int moduleid)
{
   if (cdat) free(cdat);
   if (ndat) free(ndat);
   
   cdat = malloc(screenH * screenW);
   ndat = malloc(screenH * screenW);
   
   memset(cdat, 0, screenH * screenW);
   
   for (int i = 0; i < SEEDS; i++)
   {
      int y = urand(screenH);
      int x = urand(screenW);
      cdat[y * screenW + x] = 1;
   }
   
   int scheme = urand(6);
   for (int i = 0; i <= V; i++)
   {
      double d = (double)i / (V + 2);
      uint8_t a = 255 * pow(d, 6.0);
      uint8_t b = 255 * pow(d, 2.5);
      uint8_t c = 255 * pow(d, 1.0);
      switch(scheme)
      {
         case 0: cols[i] = RGB(a, b, c); break;
         case 1: cols[i] = RGB(a, c, b); break;
         case 2: cols[i] = RGB(b, a, c); break;
         case 3: cols[i] = RGB(b, c, a); break;
         case 4: cols[i] = RGB(c, a, b); break;
         case 5: cols[i] = RGB(c, b, a); break;
      }
   }
}

int draw(int moduleid, int argc, char* argv[])
{  
  static int frame = 0;
  oscore_time now = udate();
  
  sim();
  for (int y = 0; y < screenH; y++)
  {
    for (int x = 0; x < screenW; x++)
    {
      matrix_set(x, y, cols[cdat[y * screenW + x]]);
    }
  }
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
