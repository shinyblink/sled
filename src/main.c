// Main loader.

#include <types.h>
#include <matrix.h>
#include <timers.h>

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
	printf("Cleaning up...");
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
	printf("Goodbye.\n");
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

	char moddir[] = "./modules/";
	DIR *moduledir;
	struct dirent *file;
	moduledir = opendir(moddir); // for now.
	printf("Loading modules...\n");
	if (moduledir) {
		while ((file = readdir(moduledir)) != NULL) {
			if (file->d_name[0] != '.') {
				printf("\tLoading %s...", file->d_name);
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
				printf(" Done.\n");
				modcount++;
			}
		}
		closedir(moduledir);
	} else {
		printf("Error opening modules directory. Nothing to load, nothing to try.\n");
		return 3;
	}
	printf("Loaded %i modules.\n", modcount);

	// Set up the interrupt handler.
	signal(SIGINT, interrupt);

	int i;
	printf("Initializing %i modules...", modcount);
	for (i = 0; i < modcount; i++) {
		ret = modules[i].init(i);
		if (ret != 0) {
			printf("\n");
			eprintf("Initializing module %s failed: Returned %i.", modules[i].name, ret);
			exit(6);
		}
	}
	printf(" Done.\n");

	// TODO: Run all timers, if none is available, load random page or something.
	int running = 1;
	while (running) {
		timer tnext = timer_get();
		if (tnext.moduleno == -1) {
			// Queue random.
			timer_add(utime() + RANDOM_TIME * T_SECOND, rand() % modcount);
		} else {
			wait_until(tnext.time);
			struct module mod = modules[tnext.moduleno];
			printf(">> Now drawing %s\n", mod.name);
			mod.draw();
		}
	}

	return deinit();
}
