// Module manager
// Simplistic module management.

#include "types.h"
#include "mod.h"
#include <string.h>

#define META_USED 1

#define GETMETA(mod, field) (((mod).meta & field) != 0)
#define SETMETA(mod, field) ((mod).meta |= field)
#define UNSETMETA(mod, field) ((mod).meta ^= field)

static module modules[MAX_MODULES];
static int modules_loaded = 0;

int mod_init(void) {
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

int mod_new(module newmod) {
	// TODO: Find a new slot better.
	int slot = mod_freeslot();
	if (slot != -1) {
		memcpy(&modules[slot], &newmod, sizeof(module));
		return 0;
	}

	return -1; // No slot found, oof.
}

int mod_remove(int moduleno) {
	module* mod = &modules[moduleno];

	mod->type[0] = 0;
	mod->name[0] = 0;

	mod->meta = 0;

	return 0;
}

module* mod_find(char* name) {
	int i;
	for (i = 0; i < modules_loaded; i++)
		if (strncmp(modules[i].name, name, 256) == 0)
			return &modules[i];
	return NULL;
}

module* mod_get(int moduleno) {
	if (moduleno >= MAX_MODULES || moduleno < 0)
		return NULL;
	return &modules[moduleno];
}

int mod_count(void) {
	return modules_loaded;
}

int mod_deinit(void) {
	return 0;
}
