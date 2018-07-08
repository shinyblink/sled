// Module stuff.

#include "types.h"
#include "mod.h"
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
	int modcount = mod_count();
	printf("Deinitializing %i modules...\n", modcount);
	oscore_mutex_lock(lock);
	for (i = 0; i < modcount; i++) {
		module* mod = mod_get(i);
		if (modules_specialinit(mod->type))
			continue;
		printf("\t- %s...", mod->name);
		fflush(stdout);
		ret = mod->deinit();
		loadcore_close(((mod_gfx*)mod->mod)->lib);
		if (ret != 0) {
			printf("\n");
			eprintf("Deinitializing module %s failed: Returned %i.", mod->name, ret);
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
	if (mod == NULL) {
		eprintf("\nFailed to get a free module slot for %s", name);
		return 4;
	}

	util_strlcpy(mod->type, name, 4);
	util_strlcpy(mod->name, &name[4], 256); // could malloc it, but whatever.

	// Load the module.
	loadcore_error();
	void* handle = loadcore_open(name);
	if (!handle) {
		eprintf("\nFailed to load %s: %s", name, loadcore_error());
		return 4;
	}

	mod->init = dlookup(handle, name, "init");
	mod->deinit = dlookup(handle, name, "deinit");

	if (strcmp(mod->type, "out") == 0 || strcmp(mod->type, "flt") == 0) {
		mod_out* smod = malloc(sizeof(mod_out));
		mod->mod = smod;
		smod->lib = handle;

		smod->set = dlookup(handle, name, "set");
		smod->get = dlookup(handle, name, "get");
		smod->clear = dlookup(handle, name, "clear");
		smod->render = dlookup(handle, name, "render");
		smod->getx = dlookup(handle, name, "getx");
		smod->gety = dlookup(handle, name, "gety");
		smod->wait_until = dlookup(handle, name, "wait_until");
		smod->wait_until_break = dlookup(handle, name, "wait_until_break");
	} else {
		mod_gfx* smod = malloc(sizeof(mod_gfx));
		mod->mod = smod;
		smod->lib = handle;

		// Optional!
		smod->reset = dlookup(handle, name, "reset");
		smod->draw = dlookup(handle, name, "draw");
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

		int slot = mod_freeslot();
		module* mod = mod_get(slot);
		if (modules_loadmod(mod, d_name)) {
			// Uhoh...
			printf(" Failed.\n");
			continue;
		}

		if (strcmp(mod->type, "out") == 0) {
			*outmodno = slot;
		}
		if (strcmp(mod->type, "flt") == 0) {
			filters[fltindex] = slot;
			found_filters++;
		}

		printf(" Done.\n");
	}
	*filtno = found_filters;
	asl_free_argv(dargc, dargv);

	if (mod_count() == 0) {
		eprintf("No modules found? Nothing to do, giving up on life and rendering things on matrices.\n");
		return 3;
	}

	if (*outmodno == -1) {
		eprintf("Didn't load an output module. This isn't good. ");
		return 3;
	}

	printf("Loaded %i modules.\n", mod_count());
	return 0;
}

int modules_init(int *outmodno) {
	lock = oscore_mutex_new();
	static int mod = 0;
	int ret;
	module *m;
	int modcount = mod_count();
	printf("Initializing modules...\n");
	oscore_mutex_lock(lock);
	for (; mod < mod_count(); mod++) {
		m = mod_get(mod);
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
				eprintf("Initializing module %s failed: Returned %i.", m->name, ret);
			}
			loadcore_close(((mod_gfx*)m->mod)->lib);
			mod_remove(mod);
			modcount--;
			if (mod == modcount)
				break;
			memcpy(m, m + 1, sizeof(struct module) * (mod_count() - mod));
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
