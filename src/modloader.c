// Module stuff.

#include "types.h"
//#include <timers.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "modloader.h"
#include "loadcore.h"
#include "oscore.h"
#include "asl.h"
#include "main.h"

static struct module modules[MAX_MODULES];
static int modcount = 0;
static oscore_mutex lock;

void* dlookup(void* handle, char* modname, char* name) {
	void* ptr = loadcore_sym(handle, name);
	char* error = loadcore_error();
	if (error) {
		loadcore_close(handle);
		eprintf("Failed to find symbol '%s' in %s: %s\n", name, modname, error);
		exit(5);
	}
	return ptr;
}

// These are part of the early init / late deinit passes.
// Do NOT turn this into an opt-out. Most modules expect init/deinit at the normal times.
static int modules_specialinit(const char * type) {
	if (strcmp(type, "flt") == 0)
		return 1;
	if (strcmp(type, "out") == 0)
		return 1;
	return 0;
}

int modules_deinit(void) {
	int i;
	int ret;
	printf("Deinitializing %i modules...\n", modcount);
	oscore_mutex_lock(lock);
	for (i = 0; i < modcount; i++) {
		if (modules_specialinit(modules[i].type))
			continue;
		printf("\t- %s...", modules[i].name);
		fflush(stdout);
		ret = modules[i].deinit();
		if (ret != 0) {
			printf("\n");
			eprintf("Deinitializing module %s failed: Returned %i.", modules[i].name, ret);
			return 6;
		}
		printf(" Done.\n");
	}
	printf("Done.\n");
	oscore_mutex_unlock(lock);
	oscore_mutex_free(lock);
	return 0;
}

int modules_loadmod(module* mod, char name[256]) {
	util_strlcpy(mod->type, name, 4);
	util_strlcpy(mod->name, &name[4], 256); // could malloc it, but whatever.

	// Load the module.
	loadcore_error();
	void* handle = loadcore_open(name);
	if (!handle) {
		eprintf("\nFailed to load %s: %s", name, loadcore_error());
		return 4;
	}
	mod->lib = handle;

	mod->init = dlookup(handle, name, "init");
	mod->deinit = dlookup(handle, name, "deinit");

	if (strcmp(mod->type, "out") == 0 || strcmp(mod->type, "flt") == 0) {
		mod->set = dlookup(handle, name, "set");
		mod->get = dlookup(handle, name, "get");
		mod->clear = dlookup(handle, name, "clear");
		mod->render = dlookup(handle, name, "render");
		mod->getx = dlookup(handle, name, "getx");
		mod->gety = dlookup(handle, name, "gety");
		mod->wait_until = dlookup(handle, name, "wait_until");
		mod->wait_until_break = dlookup(handle, name, "wait_until_break");
	} else {
		// Optional!
		mod->reset = loadcore_sym(handle, "reset");
		mod->draw = dlookup(handle, name, "draw");
	}
	return 0;
}

int modules_loaddir(char* moddir, char outmod_c[256], int* outmodno, char** filtnames, int* filtno, int* filters) {
	int found_filters = 0;
	printf("Loading modules...\n");
	int dargc = 0;
	char ** dargv = loadcore_init(moddir, &dargc);
	int dargi = 0;
	while (dargi < dargc) {
		char * d_name = dargv[dargi++];
		size_t len = strlen(d_name);
		printf("\t- %s...", d_name);
		fflush(stdin);
		if (len < 4) {
			printf("\n");
			eprintf("Module's name is too short to be correct.\n");
			continue;
		} else if (d_name[3] != '_') {
			printf("\n");
			eprintf("Module doesn't have a (correct) type declaration in the name\n");
			continue;
		}

		char type[4];
		util_strlcpy(type, d_name, 4);

		if (strcmp(type, "out") == 0 && strncmp(&d_name[4], outmod_c, len - 4) != 0) { // 4 for the type.
			printf(" Skipping unused output module.\n");
			continue;
		}

		int fltindex = 0;
		if (strcmp(type, "flt") == 0) {
			if (*filtno == 0) {
				printf(" Skipping unused filter modules.\n");
				continue;
			}
			char* name = &d_name[4];
			fflush(stdin);
			int i;
			int found = 0;
			for (i = 0; i < *filtno; ++i)
				if (strcmp(name, filtnames[i]) == 0) { // offset for .so
					found = 1;
					fltindex = i;
					break;
				}
			if (found == 0) {
				printf(" Skipping unused filter module.\n");
				continue;
			}
		}

		if (modules_loadmod(&modules[modcount], d_name)) {
			// Uhoh...
			printf(" Failed.\n");
			continue;
		}

		if (strcmp(modules[modcount].type, "out") == 0) {
			*outmodno = modcount;
		}
		if (strcmp(modules[modcount].type, "flt") == 0) {
			filters[fltindex] = modcount;
			found_filters++;
		}

		printf(" Done.\n");
		modcount++;
	}
	*filtno = found_filters;
	asl_free_argv(dargc, dargv);

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

int modules_init(int *outmodno) {
	lock = oscore_mutex_new();
	static int mod = 0;
	int ret;
	module *m;
	printf("Initializing modules...\n");
	oscore_mutex_lock(lock);
	for (; mod < modcount; mod++) {
		m = modules + mod;
		// Who did this!?!? This breaks basically everything.
		//if (strcmp(m->type, "gfx") != 0)
		//	continue;
		if (modules_specialinit(m->type))
			continue;
		printf("\t- %s...", m->name);
		if ((ret = m->init(mod, NULL)) == 0)
			printf(" Done.\n");
		else {
			if (ret == 1)
				printf(" Ignored by request of plugin.\n");
			else {
				printf("\n");
				eprintf("Initializing module %s failed: Returned %i.", modules[mod].name, ret);
			}
			loadcore_close(m->lib);
			modcount--;
			if (mod == modcount)
				break;
			memcpy(m, m + 1, sizeof(struct module) * (modcount - mod));
			if (*outmodno > mod) {
				outmod = modules_get(--(*outmodno));
			} else {
				mod--;
			}
		}
	}
	oscore_mutex_unlock(lock);
	printf("\nDone.");
	return 0;
}

module* modules_get(int moduleno) {
	if (moduleno > modcount || moduleno < 0)
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
