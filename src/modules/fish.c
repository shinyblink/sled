// FISh: FIFO Shell
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "timers.h"
#include "matrix.h"
#include "main.h"
#include "modloader.h"

int fish_fifo;
pthread_t fish_thread;
int fish_getch_buffer, fish_shutdown, fish_moduleno;
char fish_getch_bufferval;

void fish_panic(char * reason) {
	printf("FISh died: %s\n", reason);
	pthread_exit(0);
}

// Adds a character to a string, and disposes of the old one, unless it's NULL (in which case this creates a new one-char string)
// Can return NULL itself on malloc failure.
char * fish_growstr(char * str, char nxt) {
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
// Can return NULL itself on malloc failure.
char ** fish_growav(int argc, char ** argv, char * nxt) {
	char ** nav;
	if (argv) {
		nav = malloc(sizeof(char*) * (argc + 1));
	} else {
		nav = malloc(sizeof(char*));
	}
	if (!nav) {
		timer_free_argv(argc, argv);
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
char ** fish_pgrowav(int argc, char ** argv, char * nxt) {
	char ** nav;
	if (argv) {
		nav = malloc(sizeof(char*) * (argc + 1));
	} else {
		nav = malloc(sizeof(char*));
	}
	if (!nav) {
		timer_free_argv(argc, argv);
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
char ** fish_pnabav(int argc, char ** argv) {
	char ** nav;
	if (argv) {
		if (argc == 0)
			return NULL;
		nav = malloc(sizeof(char*) * (argc - 1));
	} else {
		return NULL;
	}
	if (!nav) {
		timer_free_argv(argc - 1, argv + 1);
		return NULL;
	}
	memcpy(nav, argv + 1, sizeof(char*) * (argc - 1));
	free(argv);
	return nav;
}

char fish_getch() {
	if (fish_getch_buffer) {
		fish_getch_buffer = 0;
		return fish_getch_bufferval;
	}
	char ch = 0;
	while (read(fish_fifo, &ch, 1) < 1)
		if (fish_shutdown)
			return 0;
	return ch;
}
void fish_ungetch(char c) {
	// "Should be impossible". But that means nothing.
	if (fish_getch_buffer)
		fish_panic("Double ungetch.");
	fish_getch_bufferval = c;
	fish_getch_buffer = 1;
}

void fish_skipws() {
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
char * fish_word(int * hitnl) {
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
			str = fish_growstr(str, c);
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
void fish_execute(char * module, int argc, char ** argv) {
	int i;
	int mcount = modules_count();
	int routing_rov = 0;
	if (!strcmp(module, "/then")) {
		// Oh, this'll be *hilarious...*
		free(module);
		if (argc != 0) {
			module = argv[0];
			argv = fish_pnabav(argc--, argv);
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
		argv = fish_pgrowav(argc++, argv, module);
		if (!argv) {
			return;
		} else {
			module = strdup("fish.so");
			if (!module) {
				timer_free_argv(argc, argv);
				return;
			}
		}
	}

	//printf("FISh: '%s', args:", module);
	//for (i = 0; i < argc; i++)
	//	printf(" '%s'", argv[i]);
	//printf("\n");

	for (i = 0; i < mcount; i++) {
		if (!strcmp(module, modules_get(i)->name)) {
			if (routing_rov) {
				main_force_random(i, argc, argv);
			} else {
				timer_add(0, i, argc, argv);
			}
			free(module);
			return;
		}
	}
	free(module);
	timer_free_argv(argc, argv);
}

void * fish_thread_func(void * arg) {
	fish_getch_buffer = 0;
	// If this doesn't work out, it'll eat more CPU than preferable (but nothing more)
	int fl = fcntl(fish_fifo, F_GETFL);
	if (fl >= 0)
		fcntl(fish_fifo, F_SETFL, fl & (~O_NONBLOCK));
	while (!fish_shutdown) {
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
			argv = fish_growav(argc++, argv, arg);
			if (!argv)
				free(argv);
		}
		if (!argv)
			argc = 0;
		// Ready.
		fish_execute(module, argc, argv);
	}
	return NULL;
}

int plugin_init(int moduleno) {
	fish_shutdown = 0;
	fish_moduleno = moduleno;

	// This is allowed to fail!
	mkfifo("sled.fish", S_IRUSR | S_IWUSR | S_IRGRP);
	// FIFO up, open it
	fish_fifo = open("sled.fish", O_RDONLY | O_NONBLOCK);
	if (fish_fifo < 0) {
		unlink("sled.fish");
		return 3;
	}
	pthread_create(&fish_thread, NULL, fish_thread_func, NULL);
	return 0;
}

int plugin_draw(int argc, char ** argv) {
	if (argc == 1) {
		// Utilities that shouldn't be part of rotation:
		if (!strcmp(argv[0], "/blank")) {
			// Blank (...forever)
			RGB col = {.red = 0, .green = 0, .blue = 0};
			matrix_fill(0, 0, MATRIX_X - 1, MATRIX_Y - 1, &col);
			matrix_render();
			char ** x = malloc(sizeof(char *));
			*x = strdup("/blank");
			timer_add(utime() + SECOND, fish_moduleno, 1, x);
			return 0;
		} else if (!strcmp(argv[0], "/error42")) {
			// Trigger error 42 as a quick escape.
			return 42;
		}
	}
	return 1;
}

int plugin_deinit() {
	fish_shutdown = 1;
	close(fish_fifo);
	unlink("sled.fish");
	pthread_join(fish_thread, NULL);
	return 0;
}
