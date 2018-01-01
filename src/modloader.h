// Module header.

#include <types.h>
typedef struct module {
	char name[255];
	char type[4];
	void *lib;

	int (*init)(int moduleno);
	int (*deinit)(void);
	int (*draw)(int argc, char* argv[]);
	int (*out_set)(byte x, byte y, RGB *color);
	int (*out_clear)(void);
	int (*out_render)(void);
} module;

void* dlookup(void* handle, char* modname, char* name);
int modules_deinit(void);
int modules_loaddir(char* moddir);
// After initial init is over, these should be readonly. Correct if incorrect, FISh assumes this. -20kdc
module* modules_get(int moduleno);
int modules_count(void);
