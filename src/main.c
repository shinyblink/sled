// Main loader.

#include <types.h>
#include <matrix.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>

struct module {
	char name[255];
	void* lib;

	int (*init)();
	int (*draw)();
	int (*deinit)();
};

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
	// TODO: Deinit plugins.
	matrix_deinit();
	printf(" Goodbye.\n");
	return 0;
}

void interrupt(int t) {
	deinit();
}

int main(int argc, char* argv[]) {
	// TODO: parse args.

	// Initialize Matrix.
	int ret = 0; //matrix_init();
	if (ret != 0) {
		// Fail.
		printf("Matrix failed to initialize.\n");
		return ret;
	}

	// TODO: Load modules.
	struct module modules[MAX_MODULES];

	char moddir[] = "./modules/";
	int modcount = 0;
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
				printf(" Done.");
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

	// TODO: Call modules' init function.
	int i;
	printf("Initializing %i modules...", modcount);
	for (i = 0; i < modcount; i++) {
		ret = modules[i].init();
		if (ret != 0) {
			printf("\n");
			eprintf("Initializing module %s failed: Returned %i.", modules[i].name, ret);
			exit(6);
		}
	}
	printf(" Done.\n");
	// TODO: Run all timers, if none is available, load random page or something.

	return deinit();
}
