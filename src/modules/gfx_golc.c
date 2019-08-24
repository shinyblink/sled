/*
  Conway's Game of Life -- COLORIZED!

  - 'Born' cells inherit a mix of the colors of their three 'parents'
  - Toroidal board (i.e. the board loops around at the edges)
  - Loop detection (i.e. the board reinitializes when the game runs into a loop)
  - Fading (i.e. cells fade in/out from one generation to the next)

  Donated to sled by orithena. Many thanks!
*/

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <random.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

// we're looping every millisecond here to accomodate the fader
#define FPS 60
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (TIME_MEDIUM * FPS)

#define GOL_ROUNDTIME_MS 768
#define GOL_MAX_REPETITIONS 5

#define GOL_LAST gol_stat.current_buf^0x01
#define GOL_CUR gol_stat.current_buf
#define GOL_HARE1 2
#define GOL_HARE2 3

static int modno;
static int frame;
static oscore_time nexttick;

static int kMatrixWidth;
static int kMatrixHeight;

/* ======= internal state of this game of life =========*/

static uint32_t gol_bufsize;
static byte* gol_stat_buf;

typedef struct GOL_InternalStatus {
  byte current_buf;
  oscore_time nextrun;
  int fadestep;
  int repetitions;
} GOL_InternalStatus;

static GOL_InternalStatus gol_stat = {
 .current_buf = 0,
 .nextrun = 0,
 .fadestep = 0,
 .repetitions = GOL_MAX_REPETITIONS
};

/*====== helper functions =====*/

/* Simulates a 3D array. Calculates the cell number in the malloc'd buffer by: buffer number, X, Y. */
static uint32_t ixy(int buffer_index, int x, int y) {
  uint32_t pos = (buffer_index * kMatrixWidth * kMatrixHeight) + (x * kMatrixHeight) + y;
  assert(pos >= 0);
  assert(pos < gol_bufsize);
  return pos;
}

/* custom modulo function -- works even for negative numbers. */
static int _mod(int x, int m) {
  while( x >= m ) x -= m;
  while( x <  0 ) x += m;
  return x;
}

/* returns the minimum of two floats */
static inline float _min(float a, float b) {
  return (a > b) ? b : a;
}

/*========== Game of Life specific helper functions =================*/

/* binary "value" of a cell for counting alive cells */
static byte gol_valueof(byte b, int x, int y) {
  // keep in mind that a cell is alive if it has a value != 0
  // keep in mind that we're operating on a toroidal board.
  return gol_stat_buf[ixy(b, _mod(x,kMatrixWidth), _mod(y,kMatrixHeight))] > 0 ? 1 : 0;
}

/* boolean representation of gol_valueof */
static bool gol_alive(byte b, int x, int y) {
  return (gol_valueof(b,x,y) & 0x01) > 0;
}

/* get color of cell */
static byte gol_colorof(byte b, int x, int y) {
  // keep in mind that a cell is dead if it's value == 0
  // A value != 0 encodes its color (hue) and that it is alive
  // keep in mind that we're operating on a toroidal board.
  return gol_stat_buf[ixy(b, _mod(x,kMatrixWidth), _mod(y,kMatrixHeight) )];
}

/* count alive neighbors */
static int gol_neighborsalive(byte b, int x, int y) {
  // keep in mind that we're operating on a toroidal board.
  int count = 0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        int v = gol_valueof(b,xi,yi);
        count += v;
      }
    }
  }
  return count;
}

/* calculate the mean of the three neighbors */
static byte gol_meanneighborcolor(byte b, int x, int y) {
  float cos_sum = 0.0;
  float sin_sum = 0.0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        if( gol_valueof(b,xi,yi) > 0 ) {
          float phi = (((float)(gol_colorof(b,xi,yi)) * M_PI) / 128.0);
          cos_sum = cos_sum + cosf(phi);
          sin_sum = sin_sum + sinf(phi);
        }
      }
    }
  }
  float phi_n = atan2(sin_sum, cos_sum);
  while( phi_n < 0.0 ) phi_n += 2*M_PI;
  byte color = (phi_n * 128.0) / M_PI;
  // color of 0 (plain red) is not allowed, that would mean that the cell is dead.
  if(color == 0) color = 255;
  return color;
}

/* reinitialize the board */
static void gol_randomize_buffers() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      gol_stat_buf[ixy(GOL_LAST, x, y)] = 0;
      gol_stat_buf[ixy(GOL_HARE2, x, y)] = 0;
      byte r;
      if( randn(1) == 0) {
        r = 0;
      } else {
        r = ( randn(2)*85 ) + 1;
      }
      gol_stat_buf[ixy(GOL_CUR, x, y)] = r;
      gol_stat_buf[ixy(GOL_HARE1, x, y)] = r;
    }
  }
  gol_stat.repetitions = 0;
}

/* compare two board buffers for equality */
static bool gol_buf_compare(int b1, int b2) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      if( (gol_stat_buf[ixy(b1, x, y)] == 0) != (gol_stat_buf[ixy(b2, x, y)] == 0) ) {
        return false;
      }
    }
  }
  return true;
}


static int gol_generation(int from, int to) {
  int currentlyalive = 0;
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      int n = gol_neighborsalive(from, x, y);
      if( gol_alive(from,x,y) ) {
				// ---------- if current cell was alive in the last iteration
        if( n < 2 ) { // underpopulation => DIE!
          gol_stat_buf[ixy(to, x, y)] = 0;
        }
        else if( n > 3 ) { // overpopulation => DIE!!1eleven!
          gol_stat_buf[ixy(to, x, y)] = 0;
        }
        else { // two or three neighbors => ha ha ha ha, stayin' alive, stayin' alive!
          gol_stat_buf[ixy(to,x,y)] = gol_stat_buf[ixy(from,x,y)];
          currentlyalive++;
        }
      }
      else {
				// ----------- if current cell was dead in the last iteration
        if( n == 3 ) { // reproduction
          gol_stat_buf[ixy(to,x,y)] = gol_meanneighborcolor(from, x, y);
          currentlyalive++;
        }
        else { // keep playing dead
          gol_stat_buf[ixy(to,x,y)] = 0;
        }
      }
    }
  }
  // actually, this return value has no use anymore. But it might be useful in the future?
  return currentlyalive;
}

static void gol_generation_control() {
  // swap the current buffer with the last buffer. The reason this works is in the #defines.
  GOL_CUR = GOL_LAST;

  // For loop detection, we use the "Hare and Tortoise" algorithm.
  // What we actually do here is running the game twice in parallel, but one at double speed;
  // i.e. for each generation in one game ("Tortoise"), we calculate two generations in
  // the other game ("Hare"). If there is a loop, the hare will run around that loop until
  // the tortoise also does enter that loop. When they meet (i.e. have the exact same
  // alive/dead state in all cells without checking colors, see gol_buf_compare()), we know
  // that the tortoise is also in the loop.
  // We allow the two to meet GOL_MAX_REPETITIONS times, so a human can see the loop.
  // This also means that a glider will have a looong screen time, especially on non-square boards.
  //  This is intentional.

  // this is the hare. it always computes two generations. It's never displayed.
  gol_generation(GOL_HARE1, GOL_HARE2);
  gol_generation(GOL_HARE2, GOL_HARE1);

  // this is the tortoise. it always computes one generation. The tortoise is displayed.
  gol_generation(GOL_LAST, GOL_CUR);

  // if hare and tortoise meet, the hare just went ahead of the tortoise through a repetition loop.
  // we only have to check the tortoise against one hare. they will meet sooner or later.
  // play the pure abstract algorithm in your head if you don't believe me.
  if( gol_buf_compare(GOL_CUR, GOL_HARE1) ) {
    gol_stat.repetitions++;
  }
}

/* fader function to fade between two generations ... and display the whole thing ^.^ */
static void gol_fader(int cstep) {
  int spd = GOL_ROUNDTIME_MS / 2;
  if( cstep >= 0  ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        byte from = gol_valueof(GOL_LAST, x, y);
        byte to = gol_valueof(GOL_CUR, x, y);

        RGB color = HSV2RGB(HSV(gol_colorof(to == 0 ? GOL_LAST : GOL_CUR, x, y), 255, (byte)((float)(from*255) + ((to*255)-(from*255)) * _min(1.0, ((float)cstep) / (float)spd))));

        matrix_set(x, y, color);

      }
    }
  }
}

/* main Game of Life control loop. controls whether to reinitialize, calculate the next generation or run the fader. */
static void gol_loop() {
  // 1024 microseconds or 1000 microseconds per millisecond, where's the difference? ;)
  oscore_time ms = udate() >> 10;

  // microsecond overflow protection -- if the gap between ms value and nextrun value is too big, we reset to sensible values.
  // this might lead to a very short generation time every 71.58278825 minutes, assuming udate works with 32 bit unsigned.
  if( gol_stat.nextrun - ms > GOL_ROUNDTIME_MS ) {
    gol_stat.nextrun = ms;
  }

  if( ms >= gol_stat.nextrun ) {
    if( gol_stat.repetitions > GOL_MAX_REPETITIONS ) {
      gol_randomize_buffers();
    }
    else {
      gol_generation_control();
    }
    gol_stat.nextrun = ms + GOL_ROUNDTIME_MS;
  }
  gol_fader( GOL_ROUNDTIME_MS-(gol_stat.nextrun - ms) );
}

/* =========== SLED module control functions ==========*/

int init(int moduleno, char* argstr) {
	// doesn't look very great with anything less.
	if (matrix_getx() < 8)
		return 1;
	if (matrix_gety() < 8)
		return 1;

	// save the matrix dimensions so we don't have to use functions inside this module
	kMatrixWidth = matrix_getx();
	kMatrixHeight = matrix_gety();

	// prepare 4 buffers -- 2 for the hare, 2 for the tortoise
	gol_bufsize = 4 * kMatrixWidth * kMatrixHeight * sizeof(byte);
	gol_stat_buf = malloc(gol_bufsize);

	// initialize the buffers with random values
	gol_randomize_buffers();

	modno = moduleno;
	return 0;
}

void reset(int _modno) {
	nexttick = udate();
	frame = 0;
}

int draw(int _modno, int argc, char* argv[]) {
	gol_loop();

	matrix_render();
	if (frame >= FRAMES) {
		frame = 0;
		return 1;
	}
	frame++;
	nexttick += FRAMETIME;
	timer_add(nexttick, modno, 0, NULL);
	return 0;
}


void deinit(int _modno) {
	free(gol_stat_buf);
}
