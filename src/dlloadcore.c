// Our own "dlopen-and-friends" wrappers.
// Useful for replacing them, as they are one of the biggest deps on a unix system,
// along with pipes.
// Plus, well, static linking?

#include <dlfcn.h>
#include "loadcore.h"
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "asl.h"

static const char * dirprefix;

void loadcore_setdir(const char* dir) {
	dirprefix = dir;
}

char ** loadcore_init(int * argcno) {
	*argcno = 0;
	struct dirent * file;
	DIR * moduledir = opendir(dirprefix); // for now.
	if (!moduledir)
		return 0;
	char ** list = 0;
	while ((file = readdir(moduledir)) != NULL) {
		int xlen = strlen(file->d_name);
		if (xlen >= 3) {
			// Check that it ends with .so
			if (strcmp(file->d_name + xlen - 3, ".so"))
				continue;
			char * p = strdup(file->d_name);
			assert(p);
			// Cut off the ".so"
			p[xlen - 3] = 0;
			list = asl_growav((*argcno)++, list, p);
		}
	}
	closedir(moduledir);
	return list;
}

void * loadcore_open(const char * modname) {
	// dirprefix + 1 "/" + modname + 3 ".so" + 1 zero terminator
	char * name2 = malloc(strlen(dirprefix) + 1 + strlen(modname) + 3 + 1);
	assert(name2);
	*name2 = 0;
	strcat(name2, dirprefix);
	strcat(name2, "/");
	strcat(name2, modname);
	strcat(name2, ".so");
	void * r = dlopen(name2, RTLD_LAZY | RTLD_GLOBAL);
	free(name2);
	return r;
}

void * loadcore_sym(void * handle, const char * name) {
	return dlsym(handle, name);
}

char * loadcore_error() {
	return dlerror();
}

void loadcore_close(void * handle) {
	dlclose(handle);
}
