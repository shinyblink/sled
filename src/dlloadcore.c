#include <dlfcn.h>
#include "loadcore.h"
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "asl.h"

void * loadcore_open(const char * modname) {
	return dlopen(modname, RTLD_LAZY | RTLD_GLOBAL);
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

char ** loadcore_getdir(char * dir, int * argcno) {
	*argcno = 0;
	struct dirent * file;
	DIR * moduledir = opendir(dir); // for now.
	if (!moduledir)
		return 0;
	char ** list = 0;
	while ((file = readdir(moduledir)) != NULL) {
		char * p = strdup(file->d_name);
		if (p) {
			list = asl_growav((*argcno)++, list, p);
		} else {
			asl_free_argv(*argcno, list);
		}
	}
	closedir(moduledir);
	return list;
}
