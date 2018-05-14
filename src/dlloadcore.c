#include <dlfcn.h>
#include "loadcore.h"

void * loadcore_open(const char * modpath) {
	return dlopen(modpath, RTLD_LAZY | RTLD_GLOBAL);
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
