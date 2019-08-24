// Module manager

#ifndef __INCLUDED_MOD__
#define __INCLUDED_MOD__

#include "asl.h"
#include "types.h"

// As of the refactor, 'module' is one struct.
// This is because otherwise we end up with way too much module-type-specific-code
//  in k2link and other module-related handlers.
#undef RGB
typedef struct module module;
struct module {
	// These two are initialized in mod_new
	// "gfx\0"
	char type[4];
	// "gfx_example\0"
	char name[256];
	// For flt, this is the next module in the output chain. This is set on load.
	int chain_link;
	// This is the second list of function declarations.
	// It must be in the order given in plugin.h,
	//  and it must be kept in sync with k2link, and mod_dl.c
	// [FUNCTION_DECLARATION_WEBRING]
	// See: plugin.h, mod.h, k2link, mod_dl.c
	int (*init)(int moduleno, char* argstr);
	void (*reset)(int moduleno);
	int (*draw)(int moduleno, int argc, char* argv[]);
	int (*set)(int moduleno, int x, int y, RGB color);
	RGB (*get)(int moduleno, int x, int y);
	int (*clear)(int moduleno);
	int (*render)(int moduleno);
	int (*getx)(int moduleno);
	int (*gety)(int moduleno);
	oscore_time (*wait_until)(int moduleno, oscore_time desired_usec);
	void (*wait_until_break)(int moduleno);
	void (*setdir)(int moduleno, const char* dir);
	int (*load)(int moduleno, module* mod, const char * name);
	void (*unload)(int moduleno, void* modloader_user);
	void (*findmods)(int moduleno, asl_av_t* result);
	void (*deinit)(int moduleno);

	// Responsible modloader. (Note: The k2link bootstrap modloader uses an index of -1.)
	int responsible_modloader;
	// Userdata owned by the mod_ module that loaded this.
	void* modloader_user;
	// Userdata owned by the module.
	void* user;
	// Is this module a valid drawable? (GFX/BGM that is initialized)
	// This is a boolean, and it's really just a safety in case someone abuses fish.
	int is_valid_drawable;
};
#define RGB(r, g, b) RGB_C(r, g, b)

// Here's a description of the current module system.
// The lifecycle of SLED's module system is:
//  Load & Init MOD (at the same time)
//  Load & Init OUT/FLT
//  Load GFX/BGM
//  -- THREAD SAFETY STARTS HERE (modules are not loaded or unloaded in this block) --
//  Init GFX/BGM
//  (run...)
//  Deinit GFX/BGM
//  -- THREAD SAFETY ENDS HERE --
//  Unload GFX/BGM
//  Deinit & Unload OUT/FLT
//  Deinit & Unload MOD

// Module IDs are essentially a 'stack'; gaps are NOT ALLOWED.
// DO NOT; REPEAT, DO NOT; CALL MODULE MANAGEMENT FUNCTIONS OUTSIDE OF MODLOADER.C!!!
// Actually, I'm moving the prototypes for *those* into modloader.c so you can't get at them.

// --
module* mod_find(const char* name);
int mod_getid(module* mod);

int mod_count();
module* mod_get(int moduleno);

#endif
