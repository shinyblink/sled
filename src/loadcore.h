// Essentially a wrapper for -ldl when possible, but might actually lead to a weird static linking system.

void loadcore_setdir(char * dir);

// Sets the current module directory. This memory is assumed to be kept around
//  for as long as modules are being loaded and unloaded.
// It also returns a list of modules in module name form.
// This can be and should be safely freed via the usual method.
char ** loadcore_init(char * dir, int* argcno);

// Emulates dlopen with a reasonable set of flags.
void * loadcore_open(const char * modpath);

// Emulates dlsym.
void * loadcore_sym(void * handle, const char * name);

// Emulates dlerror, but will return 0 even on error in some cases.
char * loadcore_error();

// Emulates dlclose.
void loadcore_close(void * handle);
