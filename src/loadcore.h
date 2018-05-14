// Essentially a wrapper for -ldl when possible, but might actually lead to a weird static linking system.

void * loadcore_open(const char * modpath);
void * loadcore_sym(void * handle, const char * name);
char * loadcore_error();
void loadcore_close(void * handle);
// NOTE: The names given here go to loadcore_open without modification.
// They can but may not have a directory prefix.
char ** loadcore_getdir(char * dir, int* argcno);
