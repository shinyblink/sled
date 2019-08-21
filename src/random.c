// Pseudo random integer generation.
// Not safe, but at least it avoids the modulo bias.
// Definitly stolen from StackOverflow, because that's how programming works nowadays.
//
// Copyright 2017, Laurence Gonsalves
// Copyright 2018, Adrian "vifino" Pistol <vifino@tty.sh>
//
// This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 
// Unported License. To view a copy of this license, visit 
// http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative 
// Commons, PO Box 1866, Mountain View, CA 94042, USA.

#include "types.h"
#include <stdlib.h>
#include "timers.h"

void random_seed(void) {
	// Dumbass way of seeding the pseudo RNG.
	srand(udate());
}

uint randn(uint n) {
	if (n == 0)
		return 0; // don't even bother.

	if (n == RAND_MAX)
		return rand();

	n++;

	// Chop off all the values that would cause skew.
	uint end = RAND_MAX / n;
	end *= n;

	// Ignore results that fall in that limit.
	// Worst case, the loop condition should fail 50% of the time.
	uint r;
	while ((r = rand()) >= end);

	return r % n;
}
