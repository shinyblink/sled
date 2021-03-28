#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define NUMPARTICLES 1000
#define FPS 60

typedef struct
{
   float x;
   float y;
   float direction;
   int species;
} particle_t;

particle_t particle[NUMPARTICLES];
static int screenW;
static int screenH;

float frand(float max)
{
   return max * rand() / (RAND_MAX + 1);
}

int validCoordinate(float x, float y)
{
   return (((int)x < screenW) && ((int)x >= 0) && ((int)y < screenH) && ((int)y >= 0));
}

float speciesValue(float sensX, float sensY, int species)
{
   RGB col = matrix_get((int)sensX, (int)sensY);
   switch (species % 3)
   {
      case 0: return col.red;
      case 1: return col.green;
      case 2: return col.blue;
   }
}

void setSpeciesValue(float sensX, float sensY, int species, unsigned char value)
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

float sensValue(particle_t *curParticle, float distance, float angle)
{
   float retVal = 0;
   float sensX = curParticle->x + distance * sinf(curParticle->direction + angle);
   float sensY = curParticle->y + distance * cosf(curParticle->direction + angle);
   if (validCoordinate(sensX, sensY))
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
   
   for (int i = 0; i < NUMPARTICLES; i++)
   {
      particle[i].x = frand((float)screenW);
      particle[i].y = frand((float)screenH);
      particle[i].direction = frand(2 * (float)M_PI);
      particle[i].species = (int)frand(3);
      //particle[i].species = (int)(0.9f + frand (1.2f));
   }
   
   return 0;
}

void reset(int moduleid)
{
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
   
   for (int i = 0; i < NUMPARTICLES; i++)
   {
      particle_t *curParticle = &particle[i];
      
      // follow own species, avoid others
      float sensLeft = sensValue(curParticle, 10, 0.6f);
      float sensRight = sensValue(curParticle, 10, -0.6f);
      curParticle->direction += 0.2f * (sensLeft - sensRight);
      curParticle->direction += 0.1f * (frand(2) - 1);
      
      // movement and border handling
      float dx = 0.5f * sinf(curParticle->direction);
      float dy = 0.5f * cosf(curParticle->direction);
      curParticle->x += dx;
      curParticle->y += dy;
      if (!validCoordinate(curParticle->x, curParticle->y))
      {
         curParticle->x -= dx;
         curParticle->y -= dy;
         curParticle->direction += (float)M_PI;
      }
      
      // drawing
      setSpeciesValue(curParticle->x, curParticle->y, curParticle->species, 255);
   }
   
   matrix_render();
   oscore_time nexttick = now + T_SECOND / FPS;
   timer_add(nexttick, moduleid, 0, NULL);
   return 0;
}

void deinit(int moduleid)
{
}