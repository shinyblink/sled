// Timers

#define SECOND 100000

typedef struct timer {
	int moduleno;
	unsigned long time;

	int argc;
	char* *argv;
} timer;

extern unsigned long utime(void);
extern unsigned long wait_until(unsigned long desired_usec);
extern int timer_add(unsigned long usec, int moduleno, int argc, char* argv[]);
extern timer timer_get(void);
