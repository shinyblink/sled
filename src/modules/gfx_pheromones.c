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
#define FOREVER 0
#define WRAP 1

typedef struct
{
   float x;
   float y;
   float direction;
   int species;
} particle_t;

static int screenW;
static int screenH;
static int frame;
static int numParticles;
static particle_t *particle;

static float frand(float max)
{
   return max * rand() / (float)RAND_MAX;
}

#if WRAP == 0
static int validCoordinate(float x, float y)
{
   return (((int)x < screenW) && ((int)x >= 0) && ((int)y < screenH) && ((int)y >= 0));
}
#endif

static float speciesValue(float sensX, float sensY, int species)
{
   RGB col = matrix_get((int)sensX, (int)sensY);
   switch (species % 3)
   {
      case 0: return col.red;
      case 1: return col.green;
      case 2: return col.blue;
   }
   return 0;
}

static void setSpeciesValue(float sensX, float sensY, int species, unsigned char value)
{
   RGB col = matrix_get((int)sensX, (int)sensY);
   switch (species % 3)
   {
      case 0:
         col.red = value;
         break;
      case 1:
         col.green = value;
         break;
      case 2:
         col.blue = value;
         break;
   }
   matrix_set((int)sensX, (int)sensY, col);
}

static float sensValue(particle_t *curParticle, float distance, float angle)
{
   float retVal = 0;
   float sensX = curParticle->x + distance * sinf(curParticle->direction + angle);
   float sensY = curParticle->y + distance * cosf(curParticle->direction + angle);
   #if (WRAP == 1)
   sensX = fmodf((sensX + screenW), screenW);
   sensY = fmodf((sensY + screenH), screenH);
   #else
   if (validCoordinate(sensX, sensY))
   #endif
   {
      float own = speciesValue(sensX, sensY, curParticle->species);
      float other = speciesValue(sensX, sensY, curParticle->species + 1);
      other += speciesValue(sensX, sensY, curParticle->species + 2);
      retVal = own - other;
   }
   return retVal / 256;
}

int init(int moduleid, char* argstr)
{
   screenW = matrix_getx();
   screenH = matrix_gety();
   numParticles = screenW * screenH / 36;
   particle = malloc(numParticles * sizeof(particle_t));
   
   return 0;
}

void reset(int moduleid)
{
   for (int i = 0; i < numParticles; i++)
   {
      particle[i].x = frand((float)screenW);
      particle[i].y = frand((float)screenH);
      particle[i].direction = frand(2 * (float)M_PI);
      particle[i].species = (int)frand(3);
   }
   
   matrix_clear();
}

int draw(int moduleid, int argc, char* argv[])
{
   oscore_time now = udate();
   for (int y = 0; y < screenH; ++y)
   {
      for (int x = 0; x < screenW; ++x)
      {
         RGB col = matrix_get(x, y);
         if(col.red) col.red--;
         if(col.green) col.green--;
         if(col.blue) col.blue--;
         matrix_set(x, y, col);
      }
   }
   
   for (int i = 0; i < numParticles; i++)
   {
      particle_t *curParticle = &particle[i];
      
      // follow own species, avoid others
      float sensLeft = sensValue(curParticle, 7, 0.3f);
      float sensRight = sensValue(curParticle, 7, -0.3f);
      curParticle->direction += 0.3f * (sensLeft - sensRight);
      #if (FOREVER == 1)
      curParticle->direction += 0.18f * (frand(2) - 1);
      #endif
      
      // movement and border handling
      float dx = 0.5f * sinf(curParticle->direction);
      float dy = 0.5f * cosf(curParticle->direction);
      #if (WRAP == 0)
      float ox = curParticle->x;
      float oy = curParticle->y;
      #endif
      curParticle->x += dx;
      curParticle->y += dy;
      #if (WRAP == 1)
      curParticle->x = fmodf((curParticle->x + screenW), screenW);
      curParticle->y = fmodf((curParticle->y + screenH), screenH);
      #else
      if (!validCoordinate(curParticle->x, curParticle->y))
      {
         curParticle->x = ox;
         curParticle->y = oy;
         curParticle->direction += (float)M_PI;
      }
      #endif
      // drawing
      setSpeciesValue(curParticle->x, curParticle->y, curParticle->species, 255);
   }
   
   matrix_render();
   if (frame++ >= FRAMES)
   {
      frame = 0;
      #if (FOREVER == 0)
      return 1;
      #endif
   }
   oscore_time nexttick = now + T_SECOND / FPS;
   timer_add(nexttick, moduleid, 0, NULL);
   return 0;
}

void deinit(int moduleid)
{
   free(particle);
   particle = NULL;
}
