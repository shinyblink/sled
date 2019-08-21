// Mandelbrot with progressive rendering (iterations)
// Positions are manually selected in the points array

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LIMIT(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

typedef struct
{
   double r;
   double i;
} complex;

typedef struct
{
   complex c; // center
   double s;  // scale
   int mi;    // max iterations
   int ipf;   // iterations per frame
   int si;    // start iteration for color scheme
   int ei;    // end iteration for color scheme
} point;

static const point points[] = {
   {{ 0.378765046933103,  0.099367336526827}, 0.000000016660047,  3000, 10,  800, 2000},
   {{ 0.270198814200415,  0.004681968467986}, 0.000000113151886, 20000, 80,  300, 5000},
   {{-0.746074740341831, -0.075444427906717}, 0.000012327288166, 20000, 80, -600, 5000},
   {{-0.198253016884858, -1.100946583302435}, 0.000000000178841,  8000, 25, 1000, 3500},
};

static const int fps = 60;
static const double er = 3.0;
static const double eg = 0.75;
static const double eb = 0.5;

static oscore_time nexttick;
static int moduleid;
static int iter;
static int w;
static int h;
static int pi;
static complex * cstore;
static double * istore;

static complex f(complex x, complex c, double *sqr)
{
   // complex numbers: x * x + c;
   complex res;
   double r2 = x.r * x.r;
   double i2 = x.i * x.i;
   res.r = r2 - i2 + c.r;
   res.i = 2 * x.r * x.i + c.i;
   *sqr = r2 + i2;
   return res;
}

static RGB color(double iter)
{
   RGB res;
   double r, g, b, c;

   c = (iter - points[pi].si) / (points[pi].ei - points[pi].si);
   r = pow(c, er);
   g = pow(c, eg);
   b = pow(c, eb);

   res.red   = LIMIT(r, 0.0, 1.0) * 255;
   res.green = LIMIT(g, 0.0, 1.0) * 255;
   res.blue  = LIMIT(b, 0.0, 1.0) * 255;
   res.alpha = 255;

   return res;
}

/*************************************************************************************/

int init(int moduleno, char* argstr)
{
   w = matrix_getx();
   h = matrix_gety();
   moduleid = moduleno;
   cstore = malloc(sizeof(complex) * w * h);
   istore = malloc(sizeof(double) * w * h);
   random_seed();
   return 0;
}

void reset(int _modno)
{
   nexttick = udate();
   memset(cstore, 0, sizeof(complex) * w * h);
   memset(istore, 0, sizeof(double) * w * h);
   matrix_clear();
   iter = 0;

   pi = randn(sizeof(points) / sizeof(point) - 1);
}

int draw(int _modno, int argc, char* argv[])
{
   for (int y = 0; y < h; ++y)
   {
      int index = y * w;
      for (int x = 0; x < w; ++x)
      {
         if (istore[index] < 1e-23)
         {
            int localiter = 0;
            complex c = points[pi].c;
            c.r += (x - w / 2) * points[pi].s / h;
            c.i += (y - h / 2) * points[pi].s / h;
            while (istore[index] < 1e-23 && localiter < points[pi].ipf)
            {
               double sqr;
               cstore[index] = f(cstore[index], c, &sqr);
               if (sqr >= 1e10)
               {
                  double diter = iter + localiter - log( 0.5 * log(sqr) / log(2) ) / log(2);
                  diter = MAX(diter, 1.01e-23);
                  istore[index] = diter;
                  matrix_set(x, y, color(istore[index]));
               }
               localiter++;
            }
         }
         index++;
      }
   }
   matrix_render();

   iter += points[pi].ipf;
   if (iter > points[pi].mi)
   {
      return 1;
   }

   nexttick += T_SECOND / fps;
   timer_add(nexttick, moduleid, 0, NULL);

   return 0;
}

void deinit(int _modno)
{
   free(cstore);
   free(istore);
}

