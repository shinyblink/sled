// Module manager
// Module management. Not so simple.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include "types.h"
#include "util.h"
#include "mod.h"
#include "oscore.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static module modules[MAX_MODULES];
static int module_count;

static int mod_freeslot(void) {
	if (module_count == MAX_MODULES)
		return -1;
	return module_count++;
}

static module* mod_getfreemod(void) {
	int slot = mod_freeslot();
	if (slot != -1)
		return &modules[slot];
	return NULL;
}

int mod_new(int loader, const char * name, int out_chain) {
	// Do very basic verification on the name, just in case.
	if (strlen(name) < 4)
		return -1;
	if (name[3] != '_')
		return -1;
	// Continue.
	int slot = mod_freeslot();
	if (slot == -1)
		return -1;
	module * mod = modules + slot;
	if (mod) {
		memset(mod, 0, sizeof(module));
		mod->chain_link = out_chain;
		mod->responsible_modloader = loader;
		util_strlcpy(mod->type, name, 4);
		util_strlcpy(mod->name, name + 4, 256);
		if (modules[loader].load(loader, mod, name)) {
			// Since this didn't load, make sure it isn't unloaded
			mod->responsible_modloader = -1;
			module_count--;
			assert(module_count == slot);
			return -1;
		}
	}
	return slot;
}

// Here's how k2link itself is loaded.
// ...You can pretty much get the picture for how k2link 'loads' other modules.
static int mod_k2link_init(int x, char * y) {
	return 0;
}
static void mod_k2link_deinit(int x) {
}
static void mod_k2link_unload(int x, void* y) {
}
static void mod_k2link_setdir(int x, const char * y) {
}
int mod_k2link_load(int, module*, const char *);
void mod_k2link_findmods(int, asl_av_t*);

int mod_new_k2link() {
	module * mod = mod_getfreemod();
	if (mod) {
		memset(mod, 0, sizeof(module));
		mod->responsible_modloader = -1;
		strcpy(mod->type, "mod");
		strcpy(mod->name, "k2link");
		mod->init = mod_k2link_init;
		mod->deinit = mod_k2link_deinit;
		mod->setdir = mod_k2link_setdir;
		mod->load = mod_k2link_load;
		mod->unload = mod_k2link_unload;
		mod->findmods = mod_k2link_findmods;
		return 0;
	}
	return 1;
}

void mod_unload_to_count(int count, int deinit, int unload) {
	for (int i = module_count - 1; i >= count; i--) {
		modules[i].is_valid_drawable = 0;
		if (deinit)
			if (modules[i].deinit)
				modules[i].deinit(i);
		if (unload) {
			int ml = modules[i].responsible_modloader;
			if (ml != -1)
				modules[ml].unload(ml, modules[i].modloader_user);
			module_count--;
		}
	}
}

module* mod_find(const char* name) {
	int i;
	for (i = 0; i < MAX_MODULES; i++)
		if (!strcmp(modules[i].name, name))
			return &modules[i];
	return NULL;
}

int mod_count() {
	return module_count;
}

module* mod_get(int moduleno) {
	if (moduleno >= MAX_MODULES || moduleno < 0)
		return NULL;
	return &modules[moduleno];
}

int mod_getid(module* mod) {
	// Yes, it actually works this way. I don't know why.
	// Presumably something to do with the way modules + 1 == &modules[1].
	return mod - modules;
}

