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
#include <loadcore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <asl.h>
#include <assert.h>
#include <timers.h>
#include <matrix.h>

#include "farbherd_lib.h"

typedef struct {
	FILE * file;
	long file_data_start;
	farbherd_header_t hdr;
	farbherd_frame_t frm;
	uint16_t * buffer;
	ulong basetick, frame;
} fh_mod_private;

#define SELFCALL module* self = mod_get(moduleno); fh_mod_private* priv = self->user;

// our fake module
static void fh_reset(int moduleno);
static int fh_init(int moduleno, char* argstr) {
	fh_reset(moduleno);
	return 0;
}
static void fh_reset(int moduleno) {
	SELFCALL
	// do whatever reset to start from frame 0
	fseek(priv->file, priv->file_data_start, SEEK_SET);
	size_t datasize = farbherd_datasize(priv->hdr.imageHead);
	if (datasize)
		memset(priv->buffer, 0, datasize);
	priv->basetick = udate();
	priv->frame = 0;
	matrix_clear();
}
static int fh_draw(int moduleno, int argc, char* argv[]) {
	SELFCALL
	// draw animation until done.
	size_t datasize = farbherd_datasize(priv->hdr.imageHead);
	if (farbherd_read_farbherd_frame(priv->file, &priv->frm, priv->hdr)) {
		return 1;
	} else {
		farbherd_apply_delta(priv->buffer, priv->frm.deltas, datasize);
		int mx = matrix_getx();
		int my = matrix_gety();
		int tx = (mx - priv->hdr.imageHead.width) / 2;
		int ty = (my - priv->hdr.imageHead.height) / 2;
		uint16_t * ptr = priv->buffer;
		for (int py = 0; py < priv->hdr.imageHead.height; py++) {
			for (int px = 0; px < priv->hdr.imageHead.width; px++) {
				RGB col = RGB(
					farbherd_be16toh(ptr[0]) >> 8, // R
					farbherd_be16toh(ptr[1]) >> 8, // G
					farbherd_be16toh(ptr[2]) >> 8  // B
				);
				matrix_set(tx + px, ty + py, col);
				ptr += 4;
			}
		}
		matrix_render();
	}
	priv->frame++;
	ulong time_ofs = (priv->hdr.frameTimeMul * priv->frame * 1000000) / priv->hdr.frameTimeDiv;
	timer_add(priv->basetick + time_ofs, moduleno, 0, NULL);
	return 0;
}
static int fh_deinit(int moduleno) {
	return 0;
}

// If you see this string something weird happened.
// For one, this is not exactly bidness done well. Sorry, vifino, I don't know if the time's available for me to learn the proper dynamic modloader stuff.
static const char * dirprefix = "I know we both believe the same thing ; No matter how dirty the bidness, do it well.";

// Loading of fh files.
static int farbherdmod_loadmod(module* mod, char name[256]) {
	if (mod == NULL) {
		eprintf("\nFailed to get a free module slot for %s", name);
		return 4;
	}

	// -- Begin actual load --
	fh_mod_private* priv = mod->user = calloc(1, sizeof(fh_mod_private));
	assert(priv);

	char * name2 = malloc(strlen(dirprefix) + 1 + strlen(name) + 1);
	assert(name2);
	*name2 = 0;
	strcat(name2, dirprefix);
	strcat(name2, "/");
	strcat(name2, name);
	priv->file = fopen(name2, "rb");
	free(name2);

	if (!priv->file) {
		free(priv);
		return 1;
	}
	if (farbherd_read_farbherd_header(priv->file, &priv->hdr)) {
		fclose(priv->file);
		free(priv);
		return 1;
	}
	if (farbherd_init_farbherd_frame(&priv->frm, priv->hdr)) {
		fclose(priv->file);
		free(priv->hdr.fileExtData);
		free(priv);
		return 1;
	}
	// Used to rewind the file.
	priv->file_data_start = ftell(priv->file);
	size_t datasize = farbherd_datasize(priv->hdr.imageHead);
	// buffer is zero-inited
	if (datasize) {
		priv->buffer = malloc(datasize);
		assert(priv->buffer);
	}
	// -- End actual load --

	util_strlcpy(mod->type, "gfx", 4);
	util_strlcpy(mod->name, name, 256); // could malloc it, but whatever.

	mod->init = fh_init;
	mod->deinit = fh_deinit;

	mod_gfx* smod = calloc(1, sizeof(mod_gfx));
	mod->mod = smod;

	smod->reset = fh_reset;
	smod->draw = fh_draw;
	return 0;
}

static int farbherdmod_loaddir(char** filtnames, int* filtno, int* filters) {
	printf("Loading modules...\n");
	int dargc = 0;
	char ** dargv = loadcore_init(&dargc);
	int dargi = 0;
	while (dargi < dargc) {
		char * d_name = dargv[dargi++];
		size_t len = strlen(d_name);
		if ((len < 3) || strcmp(&d_name[len - 3], ".fh"))
			continue;

		int slot = mod_freeslot();
		module* mod = mod_get(slot);
		if (farbherdmod_loadmod(mod, d_name)) {
			// Uhoh...
			printf(" Failed.\n");
			continue;
		}

		printf(" Done.\n");
	}
	asl_free_argv(dargc, dargv);
	return 0;
}

static void farbherdmod_setdir(const char * dir) {
	loadcore_setdir(dir);
	dirprefix = dir;
}

static mod_mod* loader;
int farbherdmod_init(void) {
	loader = calloc(1, sizeof(mod_mod));
	loader->setdir = farbherdmod_setdir;
	loader->load = farbherdmod_loadmod;
	loader->loaddir = farbherdmod_loaddir;
	module mod = { .type = "mod", .name = "farbherd", .mod = loader};

	modloader_register(mod_new(mod));
	return 0;
}

int farbherdmod_deinit(int modno) {
	free(loader);
	return 0;
}
