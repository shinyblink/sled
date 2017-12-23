// Module stuff.

#include <types.h>
//#include <timers.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct module {
	char name[255];
	void* lib;

	int (*init)(int moduleno);
	int (*draw)();
	int (*deinit)();
} module;

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

int modules_deinit() {
	int i;
	int ret;
	printf("Deinitializing %i modules...", modcount);
	for (i = 0; i < modcount; i++) {
		ret = modules[i].deinit();
		if (ret != 0) {
			printf("\n");
			eprintf("Deinitializing module %s failed: Returned %i.", modules[i].name, ret);
			return 6;
		}
	}
	printf(" Done.\n");
	return 0;
}

int modules_loaddir(char* moddir) {
	DIR *moduledir;
	struct dirent *file;
	moduledir = opendir(moddir); // for now.
	printf("Loading modules...\n");
	if (moduledir) {
		int ret;
		while ((file = readdir(moduledir)) != NULL) {
			if (file->d_name[0] != '.') {
				printf("\t- %s...", file->d_name);
				size_t len = strlen(file->d_name);
				strcpy(modules[modcount].name, file->d_name); // could malloc it, but whatever.
				char* modpath = malloc((strlen(moddir) + len) * sizeof(char));
				strcpy(modpath, moddir);
				strcpy(modpath + strlen(moddir), file->d_name);

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
		return 3;
	}
	printf("Loaded %i modules.\n", modcount);
	return 0;
}

module* modules_get(int moduleno) {
	if (moduleno > modcount)
		return NULL;
	return &modules[moduleno];
}
