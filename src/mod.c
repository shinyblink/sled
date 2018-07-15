// Module manager
// Module management. Not so simple.

#include "types.h"
#include "mod.h"
#include "oscore.h"
#include <loadcore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static module modules[MAX_MODULES];
static int modules_loaded = 0;

// Lock for not deiniting during loading.
static oscore_mutex lock;

// These are part of the early init / late deinit passes.
// Do NOT turn this into an opt-out. Most modules expect init/deinit at the normal times.
static int mod_specialinit(const char * type) {
	if (strcmp(type, "flt") == 0)
		return 1;
	if (strcmp(type, "out") == 0)
		return 1;
	return 0;
}

int mod_init(void) {
	lock = oscore_mutex_new();
	static int mod = 0;
	int ret;
	module *m;
	int modcount = mod_count();
	printf("Initializing modules...\n");
	oscore_mutex_lock(lock);
	for (; mod < modcount; mod++) {
		m = mod_get(mod);
		if (mod_specialinit(m->type))
			continue;
		printf("\t- %s...", m->name);
		if (m->init == NULL || (ret = m->init(mod, NULL)) == 0)
			printf(" Done.\n");
		else {
			if (ret == 1)
				printf(" Ignored by request of plugin.\n");
			else {
				printf("\n");
				eprintf("Initializing module %s failed: Returned %i.", m->name, ret);
			}
			mod_remove(mod);
			if (mod == modcount)
				break;
		}
	}
	oscore_mutex_unlock(lock);
	printf("\nDone.");
	return 0;
}

int mod_freeslot(void) {
	int i;
	for (i = 0; i < MAX_MODULES; i++) {
		if(!GETMETA(modules[i], META_USED)) {
			SETMETA(modules[i], META_USED);
			modules_loaded++;
			return i;
		}
	}
	return -1; // No slot found? uh oh.
}

module* mod_getfreemod(void) {
	int slot = mod_freeslot();
	if (slot != -1)
		return &modules[slot];
	return NULL;
}

module* mod_new(module newmod) {
	// TODO: Find a new slot more efficiently.
	int slot = mod_freeslot();
	if (slot != -1) {
		memcpy(&modules[slot], &newmod, sizeof(module));
		SETMETA(modules[slot], META_USED);
		return &modules[slot];
	}

	return NULL; // No slot found, oof.
}

int mod_remove(int moduleno) {
	module* mod = &modules[moduleno];

	if (mod->mod)
		free(mod->mod);

	if (GETMETA(*mod, META_FREECTX)) {
		free(mod->ctx);
	} else if (GETMETA(*mod, META_LOADCORE)) {
		loadcore_close(mod->ctx);
	}

	mod->type[0] = 0;
	mod->name[0] = 0;

	mod->meta = 0;

	modules_loaded--;

	return 0;
}

module* mod_find(char* name) {
	int i;
	for (i = 0; i < MAX_MODULES; i++)
		if (strncmp(modules[i].name, name, 256) == 0)
			return &modules[i];
	return NULL;
}

module* mod_get(int moduleno) {
	if (moduleno >= MAX_MODULES || moduleno < 0)
		return NULL;
	return &modules[moduleno];
}

int mod_getid(module* mod) {
	return (mod - (&modules[0])) / sizeof(module);
}

int mod_count(void) {
	return modules_loaded;
}

int mod_deinit(void) {
	int i;
	int modcount = mod_count();
	printf("Deinitializing %i modules...\n", modcount);
	oscore_mutex_lock(lock);
	for (i = 0; i < modcount; i++) {
		module* mod = mod_get(i);
		if (mod_specialinit(mod->type))
			continue;
		printf("\t- %s...", mod->name);
		fflush(stdout);
		int ret = 0;
		if (mod->deinit)
			ret = mod->deinit(i);

		mod_remove(i);

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

// Modloader registry.
#define MAX_MODLOADERS 8
static module* modloaders[MAX_MODLOADERS];
static int modloaders_loaded = 0;
static const char* moddir;

int modloader_register(module* loader) {
	if (modloaders_loaded >= MAX_MODLOADERS)
		return 1;
	modloaders[modloaders_loaded] = loader;
	if (moddir)
		((mod_mod*) loader->mod)->setdir(moddir);
	modloaders_loaded++;
	return 0;
}

void modloader_setdir(const char* dir) {
	int modloader;
	moddir = dir;
	for (modloader = 0; modloader < modloaders_loaded; modloader++) {
		mod_mod* loader = modloaders[modloader]->mod;
		loader->setdir(dir);
	}
}


int modloader_load(module* mod, char name[256]) {
	int status = 1;
	int modloader;
	for (modloader = 0; modloader < modloaders_loaded; modloader++) {
		mod_mod* loader = modloaders[modloader]->mod;
		status = loader->load(mod, name);
		if (status == 0)
			return 0;
	}
	return status;
}

int modloader_loaddir(char** filtnames, int* filtno, int* filters) {
	int status = 0;
	int modloader;
	for (modloader = 0; modloader < modloaders_loaded; modloader++) {
		mod_mod* loader = modloaders[modloader]->mod;
		status = loader->loaddir(filtnames, filtno, filters);
		if (status != 0)
			return status;
	}
	return status;
}

module* modloader_get(int modloader) {
	if (modloader >= MAX_MODLOADERS || modloader < 0)
		return NULL;
	return modloaders[modloader];
}

int modloader_count(void) {
	return modloaders_loaded;
}
