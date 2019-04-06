// Module stuff.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include "types.h"
#include "plugin.h"
#include "util.h"
#include "oscore.h"

PGCTX_BEGIN
	char * dirbuf;
PGCTX_END

static void* dlookup(void* handle, const char * modname, const char * name, int* fail) {
	void* ptr = dlsym(handle, name);
	char* error = dlerror();
	if (error) {
		eprintf("Failed to find symbol '%s' in %s: %s\n", name, modname, error);
		*fail = 1;
		return NULL;
	}
	return ptr;
}

int init(int _modno, char* arg) {
	PGCTX_INIT
	return 0;
}

void setdir(int _modno, const char* dir) {
	PGCTX_GET
	ctx->dirbuf = strdup(dir);
	assert(ctx->dirbuf);
}

int load(int _modno, module* mod, const char * name) {
	PGCTX_GET

	char compiled_name[512];
	snprintf(compiled_name, 512, "%s/%s.so", ctx->dirbuf, name);
	
	// Load the module.
	dlerror();
	void* handle = dlopen(compiled_name, RTLD_LAZY | RTLD_GLOBAL);
	if (!handle) {
		eprintf("\nFailed to load %s: %s\n", name, dlerror());
		return 4;
	}
	mod->modloader_user = handle;

	// [FUNCTION_DECLARATION_WEBRING]
	// See: plugin.h, mod.h, k2link, mod_dl.c

	int fail = 0;
	mod->init = dlookup(handle, name, "init", &fail);
	mod->deinit = dlookup(handle, name, "deinit", &fail);

	if ((!strcmp(mod->type, "out")) || (!strcmp(mod->type, "flt"))) {
		mod->set = dlookup(handle, name, "set", &fail);
		mod->get = dlookup(handle, name, "get", &fail);
		mod->clear = dlookup(handle, name, "clear", &fail);
		mod->render = dlookup(handle, name, "render", &fail);
		mod->getx = dlookup(handle, name, "getx", &fail);
		mod->gety = dlookup(handle, name, "gety", &fail);
		mod->wait_until = dlookup(handle, name, "wait_until", &fail);
		mod->wait_until_break = dlookup(handle, name, "wait_until_break", &fail);
	} else if (!strcmp(mod->type, "mod")) {
		mod->setdir = dlookup(handle, name, "setdir", &fail);
		mod->load = dlookup(handle, name, "load", &fail);
		mod->unload = dlookup(handle, name, "unload", &fail);
		mod->findmods = dlookup(handle, name, "findmods", &fail);
	} else {
		mod->reset = dlookup(handle, name, "reset", &fail);
		mod->draw = dlookup(handle, name, "draw", &fail);
	}
	if (fail) {
		dlclose(handle);
		return 5;
	}
	return 0;
}

void unload(int _modno, void* modloader_user) {
	dlclose(modloader_user);
}

void findmods(int _modno, asl_av_t* result) {
	PGCTX_GET
	struct dirent * file;
	DIR * moduledir = opendir(ctx->dirbuf);
	if (!moduledir)
		return;
	while ((file = readdir(moduledir)) != NULL) {
		size_t xlen = strlen(file->d_name);
		if (xlen >= 4) {
			if ((file->d_name[3] == '_') && !strcmp(file->d_name + xlen - 3, ".so")) {
				char * p = strdup(file->d_name);
				assert(p);
				// the buffer will be 3 characters longer than it should be, but that's fine
				p[xlen - 3] = 0;
				asl_growav(result, p);
			}
		}
	}
	closedir(moduledir);
}

void deinit(int _modno) {
	PGCTX_GET
	free(ctx->dirbuf);
	PGCTX_DEINIT
}

