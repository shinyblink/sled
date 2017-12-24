// Main loader.

#include <types.h>
#include <matrix.h>
#include <modloader.h>
#include <timers.h>
#include <random.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int modcount;

int deinit(void) {
	printf("Cleaning up...\n");
	int ret;
	if ((ret = modules_deinit()) != 0)
		return ret;
	if ((ret = matrix_deinit()) != 0)
		return ret;
	printf("Goodbye. :(\n");
	return 0;
}


static int running = 1;
void interrupt(int t) {
	timers_quitting = 1;
}

int pick_other(int mymodno, ulong in) {
	int mod = 0;
	if (modcount != 1)
		while ((mod = randn(modcount)) == mymodno);
	return timer_add(in, mod, 0, NULL);
}

int main(int argc, char* argv[]) {
	// TODO: parse args.

	// Initialize Matrix.
	int ret = matrix_init();
	if (ret != 0) {
		// Fail.
		printf("Matrix failed to initialize.\n");
		return ret;
	}

	// Initialize pseudo RNG.
	random_seed();

	// Load modules
	if (modules_loaddir("./modules/") != 0)
		deinit();

	modcount = modules_count();

	#ifndef PLATFORM_SDL2
	// Set up the interrupt handler. Note that SDL2 has it's own interrupt handler, so it takes precedence.
	// In both cases, the active sleep is interrupted by some method and timers_quitting is set to 1.
	signal(SIGINT, interrupt);
	#endif

	// Startup.
	timer_add(utime(), randn(modcount), 0, NULL);

	int lastmod = -1;
	while (!timers_quitting) {
		timer tnext = timer_get();
		if (tnext.moduleno == -1) {
			// Queue random.
			pick_other(lastmod, utime() + RANDOM_TIME * T_SECOND);
		} else {
			wait_until(tnext.time);
			module* mod = modules_get(tnext.moduleno);
			if (tnext.moduleno != lastmod) {
				printf("\n>> Now drawing %s", mod->name);
				fflush(stdout);
			} else {
				printf(".");
				fflush(stdout);
			};
			ret = mod->draw(tnext.argc, tnext.argv);
			lastmod = tnext.moduleno;
			if (ret != 0) {
				if (ret == 1) {
					if (lastmod != tnext.moduleno) // not an animation.
						printf("\nModule chose to pass its turn to draw.");
					pick_other(lastmod, utime() + T_MILLISECOND);
				} else {
					 eprintf("Module %s failed to draw: Returned %i", mod->name, ret);
					 deinit();
					 return 7;
				}
			}
		}
	}

	return deinit();
}
