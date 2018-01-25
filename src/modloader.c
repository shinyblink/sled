// Module stuff.

#include "types.h"
//#include <timers.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include <pthread.h>

typedef struct module {
	char name[256];
	char type[4];
	void* lib;

	int (*init)(int moduleno);
	int (*deinit)(void);
	int (*draw)(int argc, char* argv[]);
	int (*set)(int x, int y, RGB *color);
	int (*clear)(void);
	int (*render)(void);
	int (*getx)(void);
	int (*gety)(void);
	ulong (*wait_until)(ulong desired_usec);
} module;

static struct module modules[MAX_MODULES];
static int modcount = 0;
static pthread_mutex_t lock;

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

int modules_deinit(void) {
	int i;
	int ret;
	printf("Deinitializing %i modules...", modcount);
	pthread_mutex_lock(&lock);
	for (i = 0; i < modcount; i++) {
		ret = modules[i].deinit();
		if (ret != 0) {
			printf("\n");
			eprintf("Deinitializing module %s failed: Returned %i.", modules[i].name, ret);
			return 6;
		}
	}
	pthread_mutex_unlock(&lock);
	printf(" Done.\n");
	if (pthread_mutex_destroy(&lock)) {
		printf("Couldn't destroy modules mutex now no pesky background threads are around.\n");
		return 1;
	}
	return 0;
}

int modules_loadmod(module* mod, char name[256], char* modpath) {
	size_t len = strlen(name);
	util_strlcpy(mod->type, name, 4);
	util_strlcpy(mod->name, &name[4], len - 6); // could malloc it, but whatever.

	// Load the module.
	dlerror();
	void* handle = dlopen(modpath, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		eprintf("\nFailed to load %s: %s", name, dlerror());
		return 4;
	}
	mod->lib = handle;

	mod->init = dlookup(handle, modpath, "init");
	mod->deinit = dlookup(handle, modpath, "deinit");

	if (strcmp(mod->type, "out") == 0 || strcmp(mod->type, "flt") == 0) {
		mod->set = dlookup(handle, modpath, "set");
		mod->clear = dlookup(handle, modpath, "clear");
		mod->render = dlookup(handle, modpath, "render");
		mod->getx = dlookup(handle, modpath, "getx");
		mod->gety = dlookup(handle, modpath, "gety");
		mod->wait_until = dlookup(handle, modpath, "wait_until");
	} else {
		mod->draw = dlookup(handle, modpath, "draw");
	}
	return 0;
}

int modules_loaddir(char* moddir, char outmod[256], int* outmodno, char** filtnames, int* filtno, int* filters) {
	DIR *moduledir;
	struct dirent *file;
	moduledir = opendir(moddir); // for now.
	int found_filters = 0;
	printf("Loading modules...\n");
	if (moduledir) {
		while ((file = readdir(moduledir)) != NULL) {
			size_t len = strlen(file->d_name);
			if (file->d_name[0] != '.' && (strcmp(&file->d_name[len - 3], ".so") == 0)) {
				printf("\t- %s...", file->d_name);
				fflush(stdin);

				if (len < 8) {
					printf("\n");
					eprintf("Module's name is too short to be correct.\n");
					continue;
				} else if (file->d_name[3] != '_') {
					printf("\n");
					eprintf("Module doesn't have a (correct) type declaration in the name\n");
					continue;
				}

				char type[4];
				util_strlcpy(type, file->d_name, 4);

				if (strcmp(type, "out") == 0 && strncmp(&file->d_name[4], outmod, len - 4 - 3) != 0) { // 4 for the type, 3 for `.so`
					printf(" Skipping unused output module.\n");
					continue;
				}

				if (strcmp(type, "flt") == 0) {
					if (*filtno == 0) {
						printf(" Skipping unused filter modules.\n");
						continue;
					}
					char* name = &file->d_name[4];
					len = strlen(name);
					name[len - 3] = '\0';
					fflush(stdin);
					int i;
					int found = 0;
					for (i = 0; i < *filtno; ++i)
						if (strcmp(name, filtnames[i]) == 0) { // offset for .so
							found = 1;
							break;
						}
					name[len - 3] = '.';
					if (found == 0) {
						printf(" Skipping unused filter module.\n");
						continue;
					}
				}

				char* modpath = malloc((strlen(moddir) + len + 1) * sizeof(char));
				int moddirlen = strlen(moddir);
				strcpy(modpath, moddir);
				modpath[moddirlen] = '/';
				strcpy(modpath + moddirlen + 1, file->d_name);

				modules_loadmod(&modules[modcount], file->d_name, modpath);
				if (strcmp(modules[modcount].type, "out") == 0) {
					*outmodno = modcount;
				}
				if (strcmp(modules[modcount].type, "flt") == 0) {
					filters[found_filters++] = modcount;
				}

				free(modpath);

				printf(" Done.\n");
				modcount++;
			}
		}
		closedir(moduledir);
		*filtno = found_filters;
	} else {
		printf("Error opening modules directory. Nothing to load, nothing to try.\n");
		return 3;
	}

	if (modcount == 0) {
		eprintf("No modules found? Nothing to do, giving up on life and rendering things on matrices.\n");
		return 3;
	}

	if (*outmodno == -1) {
		eprintf("Didn't load an output module. This isn't good. ");
		return 3;
	}

	printf("Loaded %i modules.\n", modcount);
	return 0;
}

int modules_init(int * outmodno) {
	if (pthread_mutex_init(&lock, 0)) {
		printf("Couldn't begin to initialize modules as the background thread safety mutex couldn't be initialized.\n");
		return 1;
	}
	int mod;
	int ret;
	printf("Initializing modules...\n");
	pthread_mutex_lock(&lock);
	for (mod = 0; mod < modcount; ++mod) {
		int rerun = 1;
		while (rerun) {
			rerun = 0;
			if (strcmp(modules[mod].type, "out") != 0 && strcmp(modules[mod].type, "flt") != 0){
				printf("\t- %s...", modules[mod].name);
				ret = modules[mod].init(mod);
				if (ret > 0) {
					if (ret != 1) {
						printf("\n");
						eprintf("Initializing module %s failed: Returned %i.", modules[mod].name, ret);
					} else {
						printf(" Ignored by request of plugin.\n");
					}
					dlclose(modules[mod].lib);
					// Since this failed to init, just nuke it from orbit
					modcount--;
					if (*outmodno == mod) {
						// just in case
						eprintf("How did THIS end up the output module? Stopping.\n");
						modcount = mod;
						pthread_mutex_unlock(&lock);
						return 1;
					} else if (*outmodno > mod) {
						(*outmodno)--;
					}
					if (mod != modcount) {
						memmove(&modules[mod], &modules[mod + 1], sizeof(struct module) * (modcount - mod));
						rerun = 1;
					}
				} else {
					printf(" Done.\n");
				}
			}
		}
	}
	pthread_mutex_unlock(&lock);
	printf("\nDone.");
	return 0;
}

module* modules_get(int moduleno) {
	if (moduleno > modcount)
		return NULL;
	return &modules[moduleno];
}

module* modules_find(char* name) {
	int i;
	for (i = 0; i < modcount; ++i)
		if (strcmp(modules[i].name, name) == 0)
			return &modules[i];
	return NULL;
}

int modules_count(void) {
	return modcount;
}
