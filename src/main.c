// Main loader.

#include "types.h"
#include "matrix.h"
#include "modloader.h"
#include "timers.h"
#include "random.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>

static int modcount;

static pthread_mutex_t rmod_lock;
// Usually -1.
static int main_rmod_override = -1;
static int main_rmod_override_argc;
static char* *main_rmod_override_argv;

const char default_moduledir[] = "./modules/";
static char* modpath = NULL;

static int deinit(void) {
	printf("Cleaning up...\n");
	int ret;
	if ((ret = modules_deinit()) != 0)
		return ret;
	if ((ret = matrix_deinit()) != 0)
		return ret;
	if ((ret = timers_deinit()) != 0)
		return ret;
	pthread_mutex_destroy(&rmod_lock);
	if (main_rmod_override != -1)
		timer_free_argv(main_rmod_override_argc, main_rmod_override_argv);

	free(modpath);

	printf("Goodbye. :(\n");
	return 0;
}

static int pick_other(int mymodno, ulong in) {
	pthread_mutex_lock(&rmod_lock);
	if (main_rmod_override != -1) {
		int res = timer_add(in, main_rmod_override, main_rmod_override_argc, main_rmod_override_argv);
		main_rmod_override = -1;
		pthread_mutex_unlock(&rmod_lock);
		return res;
	}
	pthread_mutex_unlock(&rmod_lock);
	int mod = -1;
	if (modcount != 1) {
		while (mod == -1) {
			int random = randn(modcount);
			mod = random;

			// Checks after.
			if (mod == mymodno) mod = -1;
			if (strcmp(modules_get(mod)->type, "gfx") != 0) mod = -1;
		}
	} else {
		mod = 0;
		// Technically, probably always the case
		if (strcmp(modules_get(mod)->type, "gfx") != 0)
			return 1;
	}
	return timer_add(in, mod, 0, NULL);
}

void main_force_random(int mnum, int argc, char ** argv) {
	while (!timers_quitting) {
		pthread_mutex_lock(&rmod_lock);
		if (main_rmod_override == -1) {
			main_rmod_override = mnum;
			main_rmod_override_argc = argc;
			main_rmod_override_argv = argv;
			pthread_mutex_unlock(&rmod_lock);
			return;
		}
		pthread_mutex_unlock(&rmod_lock);
		usleep(5000);
	}
	// Used to prevent deadlock.
	timer_free_argv(argc, argv);
}

int usage(char* name) {
	printf("Usage: %s [-of]\n", name);
	printf("\t-m --modpath: Set directory that contains the modules to load.\n");
	printf("\t-o --output:  Set output module. Defaults to dummy.\n");
	printf("\t-f --filter:  Add a filter, can be used multiple times.\n");
	return 1;
}

static struct option longopts[] = {
	{ "modpath", required_argument, NULL, 'm' },
	{ "output",  required_argument, NULL, 'o' },
	{ "filter",  optional_argument, NULL, 'f' },
	{ NULL,      0,                 NULL, 0},
};

int main(int argc, char* argv[]) {
	int ch;
#ifdef PLATFORM_SDL2
	char outmod[256] = "sdl2";
#elif defined(PLATFORM_RPI)
	char outmod[256] = "rpi_ws2812b";
#elif defined(PLATFORM_UDP)
	char outmod[256] = "udp";
#else
	char outmod[256] = "dummy"; // dunno.
#endif

	char* filternames[MAX_MODULES];
	int filterno = 0;
	while ((ch = getopt_long(argc, argv, "m:o:f:", longopts, NULL)) != -1) {
		switch(ch) {
		case 'm': {
			int len = strlen(optarg);
			char* str = calloc(len + 1, sizeof(char));
			util_strlcpy(str, optarg, len + 1);
			modpath = str;
			break;
		}
		case 'o': {
			util_strlcpy(outmod, optarg, 256);
			break;
		}
		case 'f': {
			int len = strlen(optarg);
			char* str = malloc((len + 1) * sizeof(char));
			util_strlcpy(str, optarg, len + 1);
			filternames[filterno++] = str;
			break;
		}
		case '?':
		default:
			return usage(argv[0]);
		}
	}
	argc -= optind;
	argv += optind;

	int ret;
	ret = pthread_mutex_init(&rmod_lock, NULL);
	if (ret) {
		printf("rmod mutex failed to initialize.\n");
		return ret;
	}

	// Initialize pseudo RNG.
	random_seed();

	// Load modules
	if (modpath == NULL)
		modpath = strdup(default_moduledir);
	int* filters = NULL;
	if (filterno > 0) {
		filters = malloc(filterno * sizeof(int));
		int i;
		for (i = 0; i < filterno; ++i)
			filters[i] = -1;
	}
	int outmodno = -1;
	if ((ret = modules_loaddir(modpath, outmod, &outmodno, filternames, &filterno, filters)) != 0) {
		deinit();
		return ret;
	}

	// Initialize Timers.
	ret = timers_init(outmodno);
	if (ret) {
		printf("Timers failed to initialize.\n");
		pthread_mutex_destroy(&rmod_lock);
		return ret;
	}

	// Initialize Matrix.
	ret = matrix_init(outmodno, filters, filterno);
	if (ret) {
		// Fail.
		printf("Matrix: Output plugin failed to initialize.\n");
		timers_deinit();
		pthread_mutex_destroy(&rmod_lock);
		return ret;
	}

	// Initialize modules (this can offset outmodno)
	ret = modules_init(&outmodno);
	if (ret) {
		printf("Modules: Init failed.\n");
		return ret;
	}

	modcount = modules_count();

	// Startup.
	pick_other(-1, utime());

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
			timer_free_argv(tnext.argc, tnext.argv);
			lastmod = tnext.moduleno;
			if (ret != 0) {
				if (ret == 1) {
					if (lastmod != tnext.moduleno) // not an animation.
						printf("\nModule chose to pass its turn to draw.");
					pick_other(lastmod, utime() + T_MILLISECOND);
				} else {
					eprintf("Module %s failed to draw: Returned %i", mod->name, ret);
					timers_quitting = 1;
					deinit();
					return 7;
				}
			}
		}
	}

	return deinit();
}
