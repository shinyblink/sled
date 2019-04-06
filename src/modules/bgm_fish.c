// FISh: FIFO Shell
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

#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <oscore.h>
#include <string.h>
#include <assert.h>

#include "timers.h"
#include "matrix.h"
#include "main.h"
#include "plugin.h"

static int fish_fifo;
static oscore_task fish_task;
// NOTE: fish_shutdown is maintained by the FISh thread,
//  and is set by fish_pollshutdown.
static int fish_getch_buffer, fish_shutdown, fish_moduleno;
// Shutdown FDs, by owner. MT is main thread.
static int fish_shutdown_mt, fish_shutdown_ot;
static char fish_getch_bufferval;

static void fish_panic(char * reason) {
	printf("FISh died: %s\n", reason);
	oscore_task_exit(NULL);
}

static void fish_pollshutdown() {
	char ch = 0;
	if (read(fish_shutdown_ot, &ch, 1) > 0) {
		printf(" fish: bye! -");
		fish_shutdown = 1;
	}
}

// This is the function where FISh spends most of its time blocking.
static char fish_getch() {
	if (fish_getch_buffer) {
		fish_getch_buffer = 0;
		return fish_getch_bufferval;
	}
	char ch = 0;
	fd_set selset;
	while (read(fish_fifo, &ch, 1) < 1) {
		fish_pollshutdown();
		if (fish_shutdown)
			return 0;
		FD_ZERO(&selset);
		FD_SET(fish_fifo, &selset);
		FD_SET(fish_shutdown_ot, &selset);
		select(FD_SETSIZE, &selset, NULL, NULL, NULL);
		// printf("Select returned");
	}
	return ch;
}
static void fish_ungetch(char c) {
	// "Should be impossible". But that means nothing.
	if (fish_getch_buffer)
		fish_panic("Double ungetch.");
	fish_getch_bufferval = c;
	fish_getch_buffer = 1;
}

static void fish_skipws() {
	while (1) {
		char c = fish_getch();
		if ((c > ' ') || (c == 10)) {
			// 10 can't be consumed here, otherwise newline behavior will fail (10 will be consumed by fish_word)
			fish_ungetch(c);
			return;
		} else if (c == 0) {
			// 0 can happen on shutdown.
			return;
		}
	}
}

// A simple state machine that can parse stuff like:
// "We are all equals here, we fight for dominion tonight..."
// and "Apostrophes, such as ', are useful, but can annoy "'certain parsers that alias " and \'.'
static char * fish_word(int * hitnl) {
	char * str = NULL;
	int escape = 0;
	char quotes = 0;
	int fixempty = 0;
	while (1) {
		char c = fish_getch();
		// Again, can happen on shutdown.
		if (c == 0)
			return str;
		if ((c > ' ') || escape || quotes) {
			fixempty = 1;
			if (!escape) {
				if (c == '\\') {
					escape = 1;
					continue;
				}
				if (!quotes) {
					if (c == '\"') {
						quotes = '\"';
						continue;
					}
					if (c == '\'') {
						quotes = '\'';
						continue;
					}
				} else if (c == quotes) {
					quotes = 0;
					continue;
				}
			} else {
				escape = 0;
			}
			str = asl_growstr(str, c);
			if (!str)
				return NULL;
		} else {
			if (c == 10)
				*hitnl = 1;
			if (fixempty)
				if (!str)
					return strdup("");
			return str;
		}
	}
}

// transfers 'ownership' of args contents to this.
static void fish_execute(char * mid, asl_av_t * args) {
	int routing_rov = 0;
	if (!strcmp(mid, "/then")) {
		// Oh, this'll be *hilarious...*
		free(mid);
		mid = asl_pnabav(args);
		routing_rov = 1;
		if (!mid) {
			// argc == 0, so argv == null (unless a NULL got into the args array somehow)
			assert(args->argv);
			return;
		}
	}
	// "/then /blank" is a useful tool
	if (mid[0] == '/') {
		// "/blank" for example results in "fish /blank"
		asl_pgrowav(args, mid);
		mid = strdup("fish");
		assert(mid);
	}

	//printf("FISh: '%s', args:", mid);
	//for (int i = 0; i < args->argc; i++)
	//	printf(" '%s'", args->argv[i]);
	//printf("\n");

	// It is because of this code that thread-safety has to be ensured with proper deinit/init barriers...
	module * modref = mod_find(mid);
	free(mid);
	if (modref) {
		int i = mod_getid(modref);
		if (routing_rov) {
			main_force_random(i, args->argc, args->argv);
		} else {
			timer_add(0, i, args->argc, args->argv);
			timers_wait_until_break();
		}
		// args memory passed into main_force_random or timer_add, not our concern anymore
		return;
	}
	asl_clearav(args);
}

static void * fish_thread_func(void * arg) {
	fish_getch_buffer = 0;
	fish_shutdown = 0;
	// Make the FIFO and the shutdown pipe non-blocking.
	int fl = fcntl(fish_fifo, F_GETFL, 0);
	if (fl >= 0)
		fcntl(fish_fifo, F_SETFL, fl | O_NONBLOCK);
	fl = fcntl(fish_shutdown_ot, F_GETFL, 0);
	if (fl >= 0)
		fcntl(fish_shutdown_ot, F_SETFL, fl | O_NONBLOCK);
	while (1) {
		fish_pollshutdown();
		if (fish_shutdown)
			break;
		fish_skipws();
		int hitnl = 0;
		char * module = fish_word(&hitnl);
		if (!module)
			continue;
		asl_av_t args = {0, NULL};
		while (!hitnl) {
			fish_skipws();
			char * arg = fish_word(&hitnl);
			if (!arg)
				break;
			asl_growav(&args, arg);
		}
		// Ready.
		fish_execute(module, &args);
		oscore_task_yield();
	}
	return NULL;
}

int init(int moduleno, char* argstr) {
	fish_moduleno = moduleno;

	// This is allowed to fail!
	mkfifo("sled.fish", S_IRUSR | S_IWUSR | S_IRGRP);
	// FIFO up, open it
	fish_fifo = open("sled.fish", O_RDONLY | O_NONBLOCK);
	if (fish_fifo < 0) {
		unlink("sled.fish");
		return 3;
	}

	int tmp[2];
	if(pipe(tmp) < 0) {
		perror("bgm_fish: init tmp pipe error");
	}

	fish_shutdown_mt = tmp[1];
	fish_shutdown_ot = tmp[0];

	fish_task = oscore_task_create("bgm_fish", fish_thread_func, NULL);

	return 0;
}

int draw(int _modno, int argc, char ** argv) {
	if (argc == 1) {
		// Utilities that shouldn't be part of rotation:
		if (!strcmp(argv[0], "/blank")) {
			// Blank (...forever)
			matrix_clear();
			matrix_render();
			char ** x = malloc(sizeof(char *));
			assert(x);
			*x = strdup("/blank");
			assert(*x);
			timer_add(udate() + T_SECOND, fish_moduleno, 1, x);
			return 0;
		} else if (!strcmp(argv[0], "/error42")) {
			// Trigger error 42 as a quick escape.
			return 42;
		}
	}
	return 1;
}

void reset(int _modno) {
	// Nothing?
}

void deinit(int _modno) {
	char ch = 0;
	if(write(fish_shutdown_mt, &ch, 1) < 0) {
		perror("bgm_fish: fish_shutdown_mt write error");
	}
	oscore_task_join(fish_task);
	close(fish_fifo);
	close(fish_shutdown_mt);
	close(fish_shutdown_ot);
	unlink("sled.fish");
}
