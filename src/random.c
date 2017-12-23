// Pseudo random integer generation.
// Not safe, but at least it avoids the modulo bias.
// Definitly stolen from StackOverflow, because that's how programming works nowadays.

#include <types.h>
#include <stdlib.h>
#include <timers.h>

void random_seed(void) {
	// Dumbass way of seeding the pseudo RNG.
	srand(utime());
}

uint randn(uint n) {
	if (n == 0)
		return 0; // don't even bother.

	if ((n - 1) == RAND_MAX)
		return rand();

	// Chop off all the values that would cause skew.
	long end = RAND_MAX / n;
	end *= n;

	// Ignore results that fall in that limit.
	// Worst case, the loop condition should fail 50% of the time.
	int r;
	while ((r = rand()) >= end);

	return r % n;
}
