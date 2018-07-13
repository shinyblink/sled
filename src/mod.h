// Module manager

#ifndef __INCLUDED_MOD__
#define __INCLUDED_MOD__

#include "types.h"

typedef struct module {
	char type[4];
	char name[256];
	uint meta;
	int (*init)(int moduleno, char* argstr);
	int (*deinit)(int moduleno);

	void* mod;
} module;

int mod_init(void);
int mod_freeslot(void);
module* mod_getfreemod(void);
module* mod_new(module newmod);
int mod_remove(int moduleno);
module* mod_find(char* name);
module* mod_get(int moduleno);
int mod_getid(module* mod);
int mod_count(void);
int mod_deinit(void);

// The module loader mod loader.
typedef struct mod_mod {
	void (*setdir)(const char* dir);
	int (*load)(module* mod, char name[256]);
	int (*loaddir)(char** filtnames, int* filtno, int* filters);
} mod_mod;

int modloader_register(module* loader);
void modloader_setdir(const char* dir);
int modloader_load(module* mod, char name[256]);
int modloader_loaddir(char** filtnames, int* filtno, int* filters);
module* modloader_get(int modloader);
int modloader_count(void);

// Other module types
#undef RGB

typedef struct mod_gfx {
	void* lib;
	int (*draw)(int argc, char* argv[]);
	void (*reset)(void);
} mod_gfx;

typedef mod_gfx mod_bgm;

typedef struct mod_out {
	void* lib;
	int (*set)(int x, int y, RGB color);
	RGB (*get)(int x, int y);
	int (*clear)(void);
	int (*render)(void);
	int (*getx)(void);
	int (*gety)(void);

	ulong (*wait_until)(ulong desired_usec);
	void (*wait_until_break)();
} mod_out;

typedef mod_out mod_flt;

#define RGB(r, g, b) RGB_C(r, g, b)


#endif
