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

#include <types.h>
#include <mod.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <asl.h>

static int mx, y;

int init(int moduleno, char* argstr) {
	return 0;
}

// our fake module
int fh_init(int moduleno, char* argstr) {
	module* self = mod_get(moduleno);

	// TODO: load file into self->user.

	return 0;
}
int fh_reset(int moduleno) {
	// do whatever reset to start from frame 0
	return 0;
}
int fh_draw(int moduleno) {
	// draw animation until done.
	return 0;
}
int fh_deinit(int moduleno) {
	return 0;
}

// Loading of fh files.
int loadmod(module* mod, char name[256]) {
	if (mod == NULL) {
		eprintf("\nFailed to get a free module slot for %s", name);
		return 4;
	}

	util_strlcpy(mod->type, name, 4);
	util_strlcpy(mod->name, &name[4], 256); // could malloc it, but whatever.

	mod->init = fh_init;
	mod->deinit = fh_deinit;

	mod_gfx* smod = malloc(sizeof(mod_gfx));
	mod->mod = smod;

	smod->reset = fh_reset;
	smod->draw = fh_draw;
	return 0;
}

int loaddir(char** filtnames, int* filtno, int* filters) {
	printf("Loading modules...\n");
	int dargc = 0;
	char ** dargv = loadcore_init(&dargc);
	int dargi = 0;
	while (dargi < dargc) {
		char * d_name = dargv[dargi++];
		size_t len = strlen(d_name);
		if ((len < 3) || strcmp(&d_name[len - 3], ".fh")) {
			dargi++;
			continue;
		}

		int slot = mod_freeslot();
		module* mod = mod_get(slot);
		if (loadmod(mod, d_name)) {
			// Uhoh...
			printf(" Failed.\n");
			continue;
		}

		printf(" Done.\n");
	}
	asl_free_argv(dargc, dargv);

	printf("Loaded %i farbherd files.\n", mod_count());
	return 0;
}

int deinit(int modno) {
	free(loader);
	return 0;
}
