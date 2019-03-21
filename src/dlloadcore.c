// Our own "dlopen-and-friends" wrappers.
// Useful for replacing them, as they are one of the biggest deps on a unix system,
// along with pipes.
// Plus, well, static linking?
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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
		size_t xlen = strlen(file->d_name);
		if (xlen >= 3) {
			char * p = strdup(file->d_name);
			assert(p);
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
