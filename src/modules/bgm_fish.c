// FISh: FIFO Shell
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

#include "timers.h"
#include "matrix.h"
#include "main.h"
#include "mod.h"
#include "asl.h"

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

// transfers 'ownership' of args to this.
static void fish_execute(char * module, int argc, char ** argv) {
	int i;
	int mcount = mod_count();
	int routing_rov = 0;
	if (!strcmp(module, "/then")) {
		// Oh, this'll be *hilarious...*
		free(module);
		if (argc != 0) {
			module = argv[0];
			argv = asl_pnabav(argc--, argv);
			if (!argv) {
				free(module);
				return;
			}
			routing_rov = 1;
		} else {
			return;
		}
	}
	// "/then /blank" is a useful tool
	if (module[0] == '/') {
		// "/blank" for example results in "fish.so /blank"
		argv = asl_pgrowav(argc++, argv, module);
		if (!argv) {
			return;
		} else {
			module = strdup("fish");
			if (!module) {
				asl_free_argv(argc, argv);
				return;
			}
		}
	}

	//printf("FISh: '%s', args:", module);
	//for (i = 0; i < argc; i++)
	//	printf(" '%s'", argv[i]);
	//printf("\n");

	for (i = 0; i < mcount; i++) {
		if (!strcmp(module, mod_get(i)->name)) {
			if (routing_rov) {
				main_force_random(i, argc, argv);
			} else {
				timer_add(0, i, argc, argv);
				wait_until_break();
			}
			free(module);
			return;
		}
	}
	free(module);
	asl_free_argv(argc, argv);
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
		int argc = 0;
		char ** argv = NULL;
		while (!hitnl) {
			fish_skipws();
			char * arg = fish_word(&hitnl);
			if (!arg)
				break;
			argv = asl_growav(argc++, argv, arg);
		}
		if (!argv)
			argc = 0;
		// Ready.
		fish_execute(module, argc, argv);
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

int draw(int argc, char ** argv) {
	if (argc == 1) {
		// Utilities that shouldn't be part of rotation:
		if (!strcmp(argv[0], "/blank")) {
			// Blank (...forever)
			matrix_clear();
			matrix_render();
			char ** x = malloc(sizeof(char *));
			*x = strdup("/blank");
			timer_add(udate() + T_SECOND, fish_moduleno, 1, x);
			return 0;
		} else if (!strcmp(argv[0], "/error42")) {
			// Trigger error 42 as a quick escape.
			return 42;
		}
	}
	return 1;
}

void reset(void) {
	// Nothing?
}

int deinit() {
	char ch = 0;
	if(write(fish_shutdown_mt, &ch, 1) < 0) {
		perror("bgm_fish: fish_shutdown_mt write error");
	}
	oscore_task_join(fish_task);
	close(fish_fifo);
	close(fish_shutdown_mt);
	close(fish_shutdown_ot);
	unlink("sled.fish");
	return 0;
}
