// Main loader for sled. The entry point.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "types.h"
#include "matrix.h"
#include "mod.h"
#include "modloaders/native.h"
#include "timers.h"
#include "random.h"
#include "util.h"
#include "asl.h"
#include "oscore.h"
#include "taskpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>


static int modcount;
module* outmod;

static oscore_mutex rmod_lock;
// Usually -1.
static int main_rmod_override = -1;
static int main_rmod_override_argc;
static char* *main_rmod_override_argv;

const char default_moduledir[] = DEFAULT_MODULEDIR;
static char* modpath = NULL;

#ifdef CIMODE
static int ci_iteration_count = 0;
#endif

static int deinit(void) {
	printf("Cleaning up...\n");
	int ret;
	if ((ret = mod_deinit()) != 0)
		return ret;
	if ((ret = matrix_deinit()) != 0)
		return ret;
	if ((ret = timers_deinit()) != 0)
		return ret;
	oscore_mutex_free(rmod_lock);
	if (main_rmod_override != -1)
		asl_free_argv(main_rmod_override_argc, main_rmod_override_argv);

	taskpool_forloop_free();
	taskpool_destroy(TP_GLOBAL);

	free(modpath);

	printf("Goodbye. :(\n");
	return 0;
}

static int pick_next_random(int current_modno, ulong in) {
	oscore_mutex_lock(rmod_lock);
	if (main_rmod_override != -1) {
		int res = timer_add(in, main_rmod_override, main_rmod_override_argc, main_rmod_override_argv);
		main_rmod_override = -1;
		oscore_mutex_unlock(rmod_lock);
		return res;
	}
	oscore_mutex_unlock(rmod_lock);
	int next_mod;
	int lastvalidmod = 0;
	int usablemodcount = 0;
	for (int mod = 0; mod < modcount; mod++) {
		if (strcmp(mod_get(mod)->type, "gfx") != 0)
			continue;
		usablemodcount++;
		lastvalidmod = mod;
	}
	if (usablemodcount > 1) {
		next_mod = -1;
		while (next_mod == -1) {
			int random = randn(modcount);
			next_mod = random;

			// Checks after.
			if (next_mod == current_modno) next_mod = -1;
			module* mod = mod_get(next_mod);
			if (!mod) {
				next_mod = -1;
			} else if (strcmp(mod->type, "gfx") != 0) {
				next_mod = -1;
			}
		}
	} else if (usablemodcount == 1) {
		next_mod = lastvalidmod;
	} else {
		in += 5000000;
		next_mod = -2;
	}
	return timer_add(in, next_mod, 0, NULL);
}

static int pick_next_seq(int current_modno, ulong in) {
	oscore_mutex_lock(rmod_lock);
	if (main_rmod_override != -1) {
		int res = timer_add(in, main_rmod_override, main_rmod_override_argc, main_rmod_override_argv);
		main_rmod_override = -1;
		oscore_mutex_unlock(rmod_lock);
		return res;
	}
	oscore_mutex_unlock(rmod_lock);

	int mod, next_mod;
	int lastvalidmod = 0;
	int usablemodcount = 0;
	for (mod = 0; mod < modcount; mod++) {
		if (strcmp(mod_get(mod)->type, "gfx") != 0)
			continue;
		usablemodcount++;
		lastvalidmod = mod;
	}

	if (usablemodcount > 1) {
		next_mod = current_modno;
		int done = 0;
		while (!done) {
			next_mod++;

			//wrap around
			if (next_mod > modcount) {
				next_mod = 0;
#ifdef CIMODE
				ci_iteration_count++;
				if (ci_iteration_count > 10) { // maybe make this configurable, but its ok for now
					timers_quitting = 1;
					return 0;
				}
#endif
			}

			//found a gfx mod, take it
			if (strcmp(mod_get(next_mod)->type, "gfx") == 0) done = 1;
		}
	} else if (usablemodcount == 1) {
		next_mod = lastvalidmod;
	} else {
		in += 5000000;
		next_mod = -2;
	}
	return timer_add(in, next_mod, 0, NULL);
}

// this could also be easily rewritten to be an actual feature
static int pick_next(int current_modno, ulong in) {
#ifdef CIMODE
	return pick_next_seq(current_modno, in);
#else
	return pick_next_random(current_modno, in);
#endif
}

void main_force_random(int mnum, int argc, char ** argv) {
	while (!timers_quitting) {
		oscore_mutex_lock(rmod_lock);
		if (main_rmod_override == -1) {
			main_rmod_override = mnum;
			main_rmod_override_argc = argc;
			main_rmod_override_argv = argv;
			oscore_mutex_unlock(rmod_lock);
			return;
		}
		oscore_mutex_unlock(rmod_lock);
		usleep(5000);
	}
	// Quits out without doing anything to prevent deadlock.
	asl_free_argv(argc, argv);
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

static int interrupt_count = 0;
static void interrupt_handler(int sig) {
	//
	if (interrupt_count == 0) {
		printf("sled: Quitting due to interrupt...\n");
		timers_doquit();
	} else if (interrupt_count == 1) {
		eprintf("sled: Warning: One more interrupt until ungraceful exit!\n");
	} else {
		eprintf("sled: Instantly panic-exiting. Bye.\n");
		exit(1);
	}

	interrupt_count++;
}

int sled_main(int argc, char** argv) {
	int ch;
	char outmod_c[256] = DEFAULT_OUTMOD;

	char* filternames[MAX_MODULES];
	char* filterargs[MAX_MODULES];
	int filterno = 0;
	char* outarg = NULL;
	while ((ch = getopt_long(argc, argv, "m:o:f:", longopts, NULL)) != -1) {
		switch(ch) {
		case 'm': {
			size_t len = strlen(optarg);
			char* str = calloc(len + 1, sizeof(char));
			util_strlcpy(str, optarg, len + 1);
			modpath = str;
			break;
		}
		case 'o': {
			size_t len = strlen(optarg);
			char* tmp = malloc((len + 1) * sizeof(char));
			util_strlcpy(tmp, optarg, len + 1);
			char* arg = tmp;

			char* modname = strsep(&arg, ":");
			if (arg != NULL)
				outarg = strdup(arg);
			else
				modname = optarg;
			util_strlcpy(outmod_c, modname, 256);
			free(tmp);
			break;
		}
		case 'f': {
			char* arg = strdup(optarg);

			char* modname = strsep(&arg, ":");
			char* fltarg = NULL;
			if (arg != NULL) {
				size_t len = strlen(arg); // optarg is now the string after the colon
				fltarg = malloc((len + 1) * sizeof(char)); // i know, its a habit. a good one.
				util_strlcpy(fltarg, arg, len + 1);
			} else
				modname = optarg;
			size_t len = strlen(modname);
			char* str = malloc((len + 1) * sizeof(char));
			util_strlcpy(str, modname, len + 1);
			filternames[filterno] = str;
			filterargs[filterno] = fltarg;
			filterno++;
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
	rmod_lock = oscore_mutex_new();

	// Initialize pseudo RNG.
	random_seed();

	// Prepare for module loading
	if (modpath == NULL)
		modpath = strdup(default_moduledir);
	int* filters = NULL;
	if (filterno > 0) {
		filters = malloc(filterno * sizeof(int));
		if (filterno != 0 && !filters) {
			eprintf("Failed to malloc filter list, oops?\n");
			return 3;
		}
		int i;
		for (i = 0; i < filterno; ++i)
			filters[i] = -1;
	}

	modloader_setdir(modpath);

	// Register native module loader.
	nativemod_init();

	// Load outmod
	char outmodname[4 + ARRAY_SIZE(outmod_c)];
	snprintf(outmodname, 4 + ARRAY_SIZE(outmod_c), "out_%s", outmod_c);
	int outmodno = mod_freeslot();
	outmod = mod_get(outmodno);
	modloader_load(outmod, outmodname);
	if (outmod == NULL) {
		eprintf("Didn't load an output module. This isn't good. \n");
		deinit();
		return 3;
	};

	// Load remaining modules.
	if ((ret = modloader_loaddir(filternames, &filterno, filters)) != 0) {
		deinit();
		return ret;
	}

	// Initialize Timers.
	ret = timers_init(outmodno);
	if (ret) {
		printf("Timers failed to initialize.\n");
		oscore_mutex_free(rmod_lock);
		return ret;
	}

	// Initialize Matrix.
	ret = matrix_init(outmodno, filters, filterno, outarg, filterargs);
	if (ret) {
		// Fail.
		printf("Matrix: Output plugin failed to initialize.\n");
		timers_deinit();
		oscore_mutex_free(rmod_lock);
		return ret;
	}

	// Initialize global task pool.
	int ncpus = oscore_ncpus();
	TP_GLOBAL = taskpool_create("taskpool", ncpus, ncpus*8);

	// Initialize modules (this can offset outmodno)
	ret = mod_init();
	if (ret) {
		printf("Modules: Init failed.\n");
		return ret;
	}

	modcount = mod_count();

	signal(SIGINT, interrupt_handler);

	// Startup.
	pick_next(-1, udate());

	int lastmod = -1;
	while (!timers_quitting) {
		timer tnext = timer_get();
		if (tnext.moduleno == -1) {
			// Queue random.
			pick_next(lastmod, udate() + TIME_SHORT * T_SECOND);
		} else {
			if (tnext.time > timers_wait_until(tnext.time)) {
				// Early break. Set this timer up for elimination by any 0-time timers that have come along
				if (tnext.time == 0)
					tnext.time = 1;
				timer_add(tnext.time, tnext.moduleno, tnext.argc, tnext.argv);
				continue;
			}
			if (tnext.moduleno >= 0) {
				module* mod = mod_get(tnext.moduleno);
				mod_gfx *gfx = mod->mod;
				if (tnext.moduleno != lastmod) {
					printf("\n>> Now drawing %s", mod->name);
					fflush(stdout);
					if (gfx->reset)
						gfx->reset(tnext.moduleno);
				} else {
					printf(".");
					fflush(stdout);
				};
				ret = gfx->draw(tnext.moduleno, tnext.argc, tnext.argv);
				asl_free_argv(tnext.argc, tnext.argv);
				lastmod = tnext.moduleno;
				if (ret != 0) {
					if (ret == 1) {
						if (lastmod != tnext.moduleno) // not an animation.
							printf("\nModule chose to pass its turn to draw.");
						pick_next(lastmod, udate() + T_MILLISECOND);
						lastmod = -1;
					} else {
						eprintf("Module %s failed to draw: Returned %i", mod->name, ret);
						timers_quitting = 1;
						deinit();
						return 7;
					}
				}
			} else {
				// Virtual null module
				printf(">> using virtual null module\n");
				asl_free_argv(tnext.argc, tnext.argv);
			}
		}
	}

	return deinit();
}
