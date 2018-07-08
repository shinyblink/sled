// Module header.

#ifndef __INCLUDED_MODLOADER__
#define __INCLUDED_MODLOADER__

#include "mod.h"
#include "types.h"

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

extern void* dlookup(void* handle, char* modname, char* name);
extern int modules_deinit(void);
extern int modules_loadmod(module* mod, char name[256]);
extern int modules_loaddir(char* moddir, char outmod[256], int* outmodno, char** filtnames,
int* filtno, int* filters);
extern int modules_init(int *outmodno);
// These are not readonly during modules_init. So a mutex is held for the entirety of modules_init. - 20kdc
#define modules_get mod_get
#define modules_find mod_find
#define modules_count mod_count

#endif
