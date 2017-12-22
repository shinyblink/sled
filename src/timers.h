// Timers

#define SECOND 100000

typedef struct timer {
	int moduleno;
	unsigned long time;
} timer;

extern unsigned long utime();
extern unsigned long wait_until(unsigned long desired_usec);
extern int timer_add(unsigned long usec, int moduleno);
extern timer timer_get();
