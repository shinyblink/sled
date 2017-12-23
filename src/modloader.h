// Module header.

#include <types.h>
typedef struct module {
	char name[255];
	void *lib;

	int (*init)(int moduleno);
	int (*draw)();
	int (*deinit)();
} module;

void* dlookup(void* handle, char* modname, char* name);
int modules_deinit(void);
int modules_loaddir(char* moddir);
module* modules_get(int moduleno);
int modules_count(void);
