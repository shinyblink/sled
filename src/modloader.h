// Module header.

#ifndef __INCLUDED_MODLOADER__
#define __INCLUDED_MODLOADER__

#include <types.h>
typedef struct module {
	char name[256];
	char type[4];
	void *lib;

	int (*init)(int moduleno);
	int (*deinit)(void);
	int (*draw)(int argc, char* argv[]);
	int (*set)(int x, int y, RGB *color);
	int (*clear)(void);
	int (*render)(void);
	int (*getx)(void);
	int (*gety)(void);
	ulong (*wait_until)(ulong desired_usec);
} module;

extern void* dlookup(void* handle, char* modname, char* name);
extern int modules_deinit(void);
extern int modules_loadmod(module* mod, char name[256], char* modpath);
extern int modules_loaddir(char* moddir, char outmod[256], int* outmodno, char** filtnames, int* filtno, module** filters);
extern int modules_init(int* outmodno);
// These are not readonly during modules_init. So a mutex is held for the entirety of modules_init. - 20kdc
extern module* modules_get(int moduleno);
extern int modules_count(void);

#endif
