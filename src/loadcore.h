

void * loadcore_open(const char * modpath);
void * loadcore_sym(void * handle, const char * name);
char * loadcore_error();
void loadcore_close(void * handle);
