// ASL: Advanced String Lib
// ... what did you expect it to be?
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "timers.h"
#include "asl.h"

// Adds a character to a string, and disposes of the old one, unless it's NULL (in which case this creates a new one-char string)
// Can return NULL itself on malloc failure (in which case the original is still freed)
char * asl_growstr(char * str, char nxt) {
	char * nstr = 0;
	size_t sl = 0;
	if (str) {
		sl = strlen(str);
		nstr = malloc(sl + 2);
	} else {
		nstr = malloc(2);
	}
	if (!nstr) {
		if (str)
			free(str);
		return NULL;
	}
	if (str) {
		strcpy(nstr, str);
		free(str);
	}
	nstr[sl] = nxt;
	nstr[sl + 1] = 0;
	return nstr;
}

void asl_growav(asl_av_t * self, char * nxt) {
	self->argc++;
	// Expands NULL -> target size if necessary
	self->argv = realloc(self->argv, sizeof(char*) * self->argc);
	assert(self->argv);
	self->argv[self->argc - 1] = nxt;
}

void asl_pgrowav(asl_av_t * self, char * nxt) {
	self->argc++;
	// Expands NULL -> target size if necessary
	self->argv = realloc(self->argv, sizeof(char*) * self->argc);
	assert(self->argv);
	memmove(self->argv + 1, self->argv, sizeof(char*) * (self->argc - 1));
	self->argv[0] = nxt;
}

char * asl_pnabav(asl_av_t * self) {
	if (!self->argv) {
		return NULL;
	} else {
		char * res = self->argv[0];
		self->argc--;
		memmove(self->argv, self->argv + 1, sizeof(char*) * self->argc);
		if (self->argc) {
			self->argv = realloc(self->argv, sizeof(char*) * self->argc);
			assert(self->argv);
		} else {
			free(self->argv);
			self->argv = NULL;
		}
		return res;
	}
}

void asl_clearav(asl_av_t * self) {
	if (self->argv) {
		int i;
		for (i = 0; i < self->argc; i++)
			free(self->argv[i]);
		free(self->argv);
		self->argc = 0;
		self->argv = NULL;
	}
}

char * asl_getval(const char * key, asl_av_t * keys, asl_av_t * vals) {
	int i;
	for (i = 0; i < keys->argc; i++)
		if (!strcmp(keys->argv[i], key))
			return vals->argv[i];
	return NULL;
}

int asl_hasval(const char * key, asl_av_t * keys) {
	int i;
	for (i = 0; i < keys->argc; i++)
		if (!strcmp(keys->argv[i], key))
			return 1;
	return 0;
}

// -- IV --
// This could really do with a bit of macro-based templating,
//  since it's really a copy & paste of the AV code, but oh well

void asl_growiv(asl_iv_t * self, int nxt) {
	self->argc++;
	// Expands NULL -> target size if necessary
	self->argv = realloc(self->argv, sizeof(int) * self->argc);
	assert(self->argv);
	self->argv[self->argc - 1] = nxt;
}

void asl_pgrowiv(asl_iv_t * self, int nxt) {
	self->argc++;
	// Expands NULL -> target size if necessary
	self->argv = realloc(self->argv, sizeof(int) * self->argc);
	assert(self->argv);
	memmove(self->argv + 1, self->argv, sizeof(int) * (self->argc - 1));
	self->argv[0] = nxt;
}

int asl_pnabiv(asl_iv_t * self) {
	if (!self->argv) {
		return 0;
	} else {
		int res = self->argv[0];
		self->argc--;
		memmove(self->argv, self->argv + 1, sizeof(int) * self->argc);
		if (self->argc) {
			self->argv = realloc(self->argv, sizeof(int) * self->argc);
			assert(self->argv);
		} else {
			free(self->argv);
			self->argv = NULL;
		}
		return res;
	}
}

void asl_cleariv(asl_iv_t * self) {
	self->argc = 0;
	free(self->argv);
	self->argv = NULL;
}

void asl_test_av_validity(asl_av_t * self) {
	FILE * nope = fopen("/dev/null", "wb");
	assert(nope);
	for (int i = 0; i < self->argc; i++)
		fprintf(nope, "%p\n", self->argv[i]);
	fclose(nope);
}
void asl_test_iv_validity(asl_iv_t * self) {
	FILE * nope = fopen("/dev/null", "wb");
	assert(nope);
	for (int i = 0; i < self->argc; i++)
		fprintf(nope, "%i\n", self->argv[i]);
	fclose(nope);
}

