// Main loader.

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

struct module {
	char name[255];
	void* lib;

	int (*init)(int moduleno);
	int (*draw)();
	int (*deinit)();
};

static struct module modules[MAX_MODULES];
static int modcount;

void* dlookup(void* handle, char* modname, char* name) {
	void* ptr = dlsym(handle, name);
	char* error = dlerror();
	if (error) {
		dlclose(handle);
		eprintf("Failed to find symbol '%s' in %s: %s\n", name, modname, error);
		exit(5);
	}
	return ptr;
}

int deinit() {
	printf("Cleaning up...\n");
	int i;
	int ret;
	printf("Deinitializing %i modules...", modcount);
	for (i = 0; i < modcount; i++) {
		ret = modules[i].deinit();
		if (ret != 0) {
			printf("\n");
			eprintf("Deinitializing module %s failed: Returned %i.", modules[i].name, ret);
			exit(6);
		}
	}
	printf(" Done.\n");
	matrix_deinit();
	printf("Goodbye. :(\n");
	return 0;
}

void interrupt(int t) {
	deinit();
	exit(1);
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
	char moddir[] = "./modules/";
	DIR *moduledir;
	struct dirent *file;
	moduledir = opendir(moddir); // for now.
	printf("Loading modules...\n");
	if (moduledir) {
		while ((file = readdir(moduledir)) != NULL) {
			if (file->d_name[0] != '.') {
				printf("\t- %s...", file->d_name);
				size_t len = strlen(file->d_name);
				strcpy(modules[modcount].name, file->d_name); // could malloc it, but whatever.
				char* modpath = malloc((sizeof(moddir) + len) * sizeof(char));
				strcpy(modpath, moddir);
				strncpy(modpath+sizeof(moddir) - 1, file->d_name, len);

				// Load the module.
				dlerror();
				void* handle = dlopen(modpath, RTLD_LAZY | RTLD_GLOBAL);
				if (!handle) {
					eprintf("\nFailed to load %s: %s", file->d_name, dlerror());
					return 4;
				}
				modules[modcount].lib = handle;

				modules[modcount].init = dlookup(handle, modpath, "plugin_init");
				modules[modcount].draw = dlookup(handle, modpath, "plugin_draw");
				modules[modcount].deinit = dlookup(handle, modpath, "plugin_deinit");

				free(modpath);

				ret = modules[modcount].init(modcount);
				if (ret > 0) {
					if (ret != 1) {
						printf("\n");
						eprintf("Initializing module %s failed: Returned %i.", file->d_name, ret);
					} else {
						printf(" Ignored by request of plugin.\n");
					}
					dlclose(handle);
				} else {
					printf(" Done.\n");
					modcount++;
				}
			}
		}
		closedir(moduledir);
	} else {
		printf("Error opening modules directory. Nothing to load, nothing to try.\n");
		return 3;
	}

	if (modcount == 0) {
		eprintf("No modules found? Nothing to do, giving up on life and rendering things on matrices.\n");
		deinit();
		return 3;
	}
	printf("Loaded %i modules.\n", modcount);

	// Set up the interrupt handler.
	signal(SIGINT, interrupt);

	// TODO: Run all timers, if none is available, load random page or something.
	int running = 1;
	while (running) {
		timer tnext = timer_get();
		if (tnext.moduleno == -1) {
			// Queue random.
			timer_add(utime() + RANDOM_TIME * T_SECOND, randn(modcount));
		} else {
			wait_until(tnext.time);
			struct module mod = modules[tnext.moduleno];
			printf(">> Now drawing %s\n", mod.name);
			ret = mod.draw();
			if (ret != 0) {
				if (ret == 1) {
					printf("Module did not want to draw. Is it okay? Repicking.\n");
					timer_add(utime() + T_MILLISECOND, randn(modcount));
				} else {
					 eprintf("Module %s failed to draw: Returned %i", mod.name, ret);
					 deinit();
					 exit(7);
				}
			}
		}
	}

	return deinit();
}
