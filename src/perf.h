
#include "types.h"
#include "timers.h"
#include <stdio.h>

#ifdef PERF
// one timer for each module, assuming there are never more than 64 modules loaded
static oscore_time perf_timer[128];
#endif

static inline void perf_start(int n) {
#ifdef PERF
  perf_timer[n*2] = udate();
  perf_timer[(n*2)+1] = perf_timer[n*2];
#endif
}

static inline void perf_print(int n, char* msg) {
#ifdef PERF
  oscore_time cur = udate();
  printf("\t%02d: %16lu %16lu - %s\n", n, cur - perf_timer[n*2], cur - perf_timer[(n*2)+1], msg);
  perf_timer[(n*2)+1] = udate();
#endif
}
