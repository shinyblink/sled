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


void interrupt(int t) {
	deinit();
	exit(1);
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

	// Set up the interrupt handler.
	signal(SIGINT, interrupt);

	// Startup.
	timer_add(utime(), randn(modcount), 0, NULL);

	int running = 1;
	while (running) {
		timer tnext = timer_get();
		if (tnext.moduleno == -1) {
			// Queue random.
			pick_other(tnext.moduleno, utime() + RANDOM_TIME * T_SECOND);
		} else {
			wait_until(tnext.time);
			module* mod = modules_get(tnext.moduleno);
			printf(">> Now drawing %s\n", mod->name);
			ret = mod->draw();
			if (ret != 0) {
				if (ret == 1) {
					printf("Module did not want to draw. Is it okay? Repicking.\n");
					pick_other(tnext.moduleno, utime() + T_MILLISECOND);
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
