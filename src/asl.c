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

// Adds an argument to an argv, and disposes of the old one, unless it's NULL.
// Element pointers are copied (no strdups).
// Can return NULL itself on malloc failure (in which case everything is freed)
char ** asl_growav(int argc, char ** argv, char * nxt) {
	char ** nav;
	if (argv) {
		nav = malloc(sizeof(char*) * (argc + 1));
	} else {
		nav = malloc(sizeof(char*));
	}
	if (!nav) {
		asl_free_argv(argc, argv);
		free(nxt);
		return NULL;
	}
	if (argv) {
		memcpy(nav, argv, sizeof(char*) * argc);
		free(argv);
	}
	nav[argc] = nxt;
	return nav;
}

// Like the previous, but prepending.
char ** asl_pgrowav(int argc, char ** argv, char * nxt) {
	char ** nav;
	if (argv) {
		nav = malloc(sizeof(char*) * (argc + 1));
	} else {
		nav = malloc(sizeof(char*));
	}
	if (!nav) {
		asl_free_argv(argc, argv);
		free(nxt);
		return NULL;
	}
	if (argv) {
		memcpy(nav + 1, argv, sizeof(char*) * argc);
		free(argv);
	}
	nav[0] = nxt;
	return nav;
}

// Like the previous, but removing the first, not adding to it.
// It's assumed you've "collected" it elsewhere.
char ** asl_pnabav(int argc, char ** argv) {
	char ** nav;
	if (argv) {
		if (argc == 0)
			return NULL;
		nav = malloc(sizeof(char*) * (argc - 1));
	} else {
		return NULL;
	}
	if (!nav) {
		asl_free_argv(argc - 1, argv + 1);
		return NULL;
	}
	memcpy(nav, argv + 1, sizeof(char*) * (argc - 1));
	free(argv);
	return nav;
}

void asl_free_argv(int argc, char ** argv) {
	if (argv) {
		int i;
		for (i = 0; i < argc; i++)
			free(argv[i]);
		free(argv);
	}
}
