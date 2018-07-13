// Module stuff.

#include "../types.h"
#include "../mod.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../util.h"
#include "native.h"
#include "../loadcore.h"
#include "../oscore.h"
#include "../asl.h"


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


int native_loadmod(module* mod, char name[256]) {
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

		smod->reset = dlookup(handle, name, "reset");
		smod->draw = dlookup(handle, name, "draw");
	}
	return 0;
}

int native_loaddir(char** filtnames, int* filtno, int* filters) {
	int found_filters = 0;
	printf("Loading modules...\n");
	int dargc = 0;
	char ** dargv = loadcore_init(&dargc);
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

		if (strcmp(type, "gfx") != 0 && strcmp(type, "flt") != 0)
			continue;

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
		if (native_loadmod(mod, d_name)) {
			// Uhoh...
			printf(" Failed.\n");
			continue;
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

	printf("Loaded %i modules.\n", mod_count());
	return 0;
}

mod_mod* loader;
int nativemod_init(void) {
	loader = calloc(1, sizeof(mod_mod));
	loader->setdir = loadcore_setdir;
	loader->load = native_loadmod;
	loader->loaddir = native_loaddir;
	module mod = { .type = "mod", .name = "native", .mod = loader};

	modloader_register(mod_new(mod));
	return 0;
}

int nativemod_deinit(int modno) {
	free(loader);
	return 0;
}
