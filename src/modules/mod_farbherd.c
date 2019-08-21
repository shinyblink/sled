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
#include <plugin.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util.h>
#include <asl.h>
#include <assert.h>
#include <timers.h>
#include <matrix.h>
#include <dirent.h>

#include <ext/farbherd.h>

typedef struct {
	FILE * file;
	long file_data_start;
	farbherd_header_t hdr;
	farbherd_frame_t frm;
	uint16_t * buffer;
	oscore_time basetick;
	int frame;
} fh_mod_private;

#define SELFCALL module* self = mod_get(moduleno); fh_mod_private* priv = self->modloader_user;

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
	oscore_time time_ofs = (1000000ull * priv->hdr.frameTimeMul * priv->frame) / priv->hdr.frameTimeDiv;
	timer_add(priv->basetick + time_ofs, moduleno, 0, NULL);
	return 0;
}
static void fh_deinit(int moduleno) {
}

// --------------------------------------------------

PGCTX_BEGIN
	char * dirbuf;
PGCTX_END

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

	if (strlen(name) < 4)
		return 1;
	if (memcmp(name, "gfx_", 4))
		return 1;
	name += 4;

	// -- Begin actual load --
	fh_mod_private* priv = mod->modloader_user = calloc(1, sizeof(fh_mod_private));
	assert(priv);

	char * name2 = malloc(strlen(ctx->dirbuf) + 1 + strlen(name) + 3 + 1);
	assert(name2);
	*name2 = 0;
	strcat(name2, ctx->dirbuf);
	strcat(name2, "/");
	strcat(name2, name);
	strcat(name2, ".fh");
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

	mod->init = fh_init;
	mod->deinit = fh_deinit;
	mod->reset = fh_reset;
	mod->draw = fh_draw;
	return 0;
}

void unload(int _modno, void* modloader_user) {
	fh_mod_private* priv = modloader_user;
	fclose(priv->file);
	free(priv->hdr.fileExtData);
	free(priv->frm.frameExtData);
	free(priv->frm.deltas);
	free(priv->buffer);
	free(priv);
}

void findmods(int _modno, asl_av_t* result) {
	PGCTX_GET
	struct dirent * file;
	DIR * moduledir = opendir(ctx->dirbuf);
	if (!moduledir)
		return;
	while ((file = readdir(moduledir)) != NULL) {
		size_t xlen = strlen(file->d_name);
		if (xlen >= 3) {
			if (!strcmp(file->d_name + (xlen - 3), ".fh")) {
				// Add room for the 'gfx_' and a zero byte.
				char * p = malloc(strlen(file->d_name) + 5);
				assert(p);
				strcpy(p, "gfx_");
				strcpy(p + 4, file->d_name);
				// Cut off the '.fh'.
				p[xlen + 4 - 3] = 0;
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
