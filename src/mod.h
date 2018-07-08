// Module manager

#ifndef __INCLUDED_MOD__
#define __INCLUDED_MOD__

typedef struct module {
	char type[4];
	char name[256];
	int meta;
	int (*init)(int moduleno, char* argstr);
	int (*deinit)(void);

	void* mod;
} module;

int mod_init(void);
int mod_freeslot(void);
module* mod_getfreemod(void);
int mod_new(module newmod);
int mod_remove(int moduleno);
module* mod_find(char* name);
module* mod_get(int moduleno);
int mod_count(void);
int mod_deinit(void);

#endif
