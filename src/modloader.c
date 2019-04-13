#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "asl.h"
#include "mod.h"
#include "modloader.h"

// This memory is managed in main.c...
char* modloader_modpath = NULL;

// This isn't, but is used by main.c
asl_iv_t modloader_gfx_rotation = {0, NULL};

// ---- mod.c prototypes
// Given a loader module ID and details on what to load, load a module.
// Sets up everything *ready* for init, but doesn't actually do the init.
// Returns the slot, or -1 on failure.
int mod_new(int loader, const char * name, int out_chain);
// Returns non-zero on failure.
int mod_new_k2link();

// Runs some process on the modules from the last loaded
//  down to the module with the ID in 'count'.
// If 'deinit' is non-zero, deinits the module.
// If 'unload' is non-zero, actually performs the unload and thus reduces module count.
// Combinations of these are used by modloader.c to implement the unload passes described above.
// Returns non-zero on error.
void mod_unload_to_count(int count, int deinit, int unload);
// ----

static asl_av_t all_gfxbgm = {0, NULL};

int modloader_initmod() {
	puts("Initializing modloader tree...");
	asl_av_t done = {0, NULL};
	// Inject k2link.
	mod_new_k2link();
	// Now continue...
	// This whole thing WILL still need a rewrite if slots can contain gaps and gaps can be reused.
	// Right now, the load is done in essentially a breadth-first order.
	// Each pass finds all the mods loaded in the previous pass,
	//  and if they're modloaders, sets their directory and loads in more modloaders.
	// Since k2link is a mod, that's the output of the 'first pass'.
	// 'beginning' increases, ensuring older passes aren't checked again.
	// Furthermore, name uniqueness is ensured on modloaders via the 'done' list.
	int beginning = 0;
	while (1) {
		int didsomething = 0;
		int mc = mod_count();
		for (int loader = beginning; loader < mc; loader++) {
			module * v = mod_get(loader);
			if (!strcmp(v->type, "mod")) {
				asl_av_t avl = {0, NULL};
				v->setdir(loader, modloader_modpath);
				v->findmods(loader, &avl);
				for (int ac = 0; ac < avl.argc; ac++) {
					if (!asl_hasval(avl.argv[ac], &done)) {
						if (((avl.argv[ac][0] == 'g') && (avl.argv[ac][1] == 'f') && (avl.argv[ac][2] == 'x')) ||
						    ((avl.argv[ac][0] == 'b') && (avl.argv[ac][1] == 'g') && (avl.argv[ac][2] == 'm'))) {
							// Register GFX/BGM module?
							if (!asl_hasval(avl.argv[ac], &all_gfxbgm)) {
								char * nt = strdup(avl.argv[ac]);
								assert(nt);
								asl_growav(&all_gfxbgm, nt);
							}
						}
						if ((avl.argv[ac][0] == 'm') && (avl.argv[ac][1] == 'o') && (avl.argv[ac][2] == 'd')) {
							printf("%s -> %s\n", v->name, avl.argv[ac]);
							// Another modloader to add to the pile?
							int nml = mod_new(loader, avl.argv[ac], 0);
							if (nml != -1) {
								if (mod_get(nml)->init(nml, NULL)) {
									// >:(
									mod_unload_to_count(nml, 0, 1);
								} else {
									// Apparently so!
									char * ne = strdup(avl.argv[ac]);
									assert(ne);
									asl_growav(&done, ne);
									didsomething = 1;
								}
							}
						}
					}
				}
				asl_clearav(&avl);
			}
		}
		if (!didsomething)
			break;
		// Make sure that we begin at the first module loaded after the current set of loaders,
		//  so loaders aren't repeated.
		beginning = mc;
	}
	asl_clearav(&done);
	return 0;
}

static int modloader_load(const char * name, int chain_link) {
	int mc = mod_count();
	for (int loader = 0; loader < mc; loader++) {
		module * v = mod_get(loader);
		if (!strcmp(v->type, "mod")) {
			int v2 = mod_new(loader, name, chain_link);
			if (v2 != -1) {
				printf("%s -> %s\n", v->name, name);
				return v2;
			}
		}
	}
	return -1;
}

// NOTE! THIS IS USED ONLY FOR OUT/FLT.
static int modloader_load_and_init(const char * name, char * args, int chain_link) {
	int v2 = modloader_load(name, chain_link);
	if (v2 != -1) {
		int initstat = mod_get(v2)->init(v2, args);
		if (initstat) {
			// Load failure. Get rid of the module.
			mod_unload_to_count(v2, 0, 1);
			return -1;
		}
		return v2;
	}
	free(args);
	return -1;
}

int modloader_initout(asl_av_t* flt_names, asl_av_t* flt_args) {
	// Used to revert
	int backup_modcount = mod_count();
	puts("Initializing output module and filters...");
	int current_outmod = -1;
	while (flt_names->argc > 0) {
		char * nam = asl_pnabav(flt_names);
		int val = modloader_load_and_init(nam, asl_pnabav(flt_args), current_outmod);
		if (val == -1) {
			printf("%s had a load error...\n", nam);
			mod_unload_to_count(backup_modcount, 1, 1);
			free(nam);
			return -1;
		} else {
			current_outmod = val;
			free(nam);
		}
	}
	return current_outmod;
}
// -- GFX/BGM init/deinit (the difficult bit) --

static int modloader_pregfx_mod_count;
static asl_iv_t modloader_gfx_inited;

int modloader_initgfx(void) {
	modloader_pregfx_mod_count = mod_count();
	asl_iv_t loaded = {0, NULL};
	for (int i = 0; i < all_gfxbgm.argc; i++) {
		int ld = modloader_load(all_gfxbgm.argv[i], -1);
		if (ld == -1) {
			printf("%s had a load error...\n", all_gfxbgm.argv[i]);
			mod_unload_to_count(modloader_pregfx_mod_count, 0, 1);
			asl_cleariv(&loaded);
			return 1;
		}
		asl_growiv(&loaded, ld);
	}
	puts("initializing");
	for (int i = 0; i < loaded.argc; i++) {
		module * mod = mod_get(loaded.argv[i]);
		puts(mod->name);
		if (mod->init(loaded.argv[i], NULL)) {
			puts("...did not init");
		} else {
			puts("...did init");
			mod->is_valid_drawable = 1;
			// Bring GFX modules into rotation (see mod.h for details on this field)
			if (!strcmp(mod->type, "gfx")) {
				puts("...and has rotation privs");
				asl_growiv(&modloader_gfx_rotation, loaded.argv[i]);
			}
			asl_pgrowiv(&modloader_gfx_inited, loaded.argv[i]);
		}
	}
	asl_cleariv(&loaded);
	return 0;
}
void modloader_deinitgfx(void) {
	asl_cleariv(&modloader_gfx_rotation);
	for (int i = 0; i < modloader_gfx_inited.argc; i++) {
		int mid = modloader_gfx_inited.argv[i];
		module * mod = mod_get(mid);
		mod->deinit(mid);
	}
	asl_cleariv(&modloader_gfx_inited);
	mod_unload_to_count(modloader_pregfx_mod_count, 0, 1);
}
// -- Remaining deinit --
void modloader_deinitend(void) {
	mod_unload_to_count(0, 1, 1);
	asl_clearav(&all_gfxbgm);
}

