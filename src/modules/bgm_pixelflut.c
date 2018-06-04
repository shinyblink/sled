// OPC: Open Pixel Control output
// This allows using SLED as an OPC output.
// Shoudl an OPC connection connect,
//  the module will start.
// Should all OPC connections disconnect,
//  the module will end.

#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <oscore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "timers.h"
#include "matrix.h"
#include "main.h"
#include "modloader.h"
#include "asl.h"

// It is *IMPOSSIBLE* for a px client to send a length that escapes this buffer.
static byte px_array[65536];

static int px_shutdown_fd_mt, px_shutdown_fd_ot;
// px_mtcountdown is the time until we decide to end. It's main-thread-only.
static int px_moduleno, px_mtcountdown;
static int px_pixelcount, px_clientcount;

static int px_mx, px_my;
static ulong px_mtlastframe;
static oscore_task px_task;

#define PX_MTCOUNTDOWN_MAX 100
#define FRAMETIME 10000
#define PX_PORT 1337
//#define PX_SNAKE

typedef struct {
	int socket; // The socket
	int off_x;
	int off_y;
	void * next; // The next client
} px_client_t;

// shamelessly ripped from pixelnuke
static inline int fast_str_startswith(const char* prefix, const char* str) {
	char cp, cs;
	while ((cp = *prefix++) == (cs = *str++)) {
		if (cp == 0)
			return 1;
	}
	return !cp;
}

// Decimal string to unsigned int. This variant does NOT consume +, - or whitespace.
// If **endptr is not NULL, it will point to the first non-decimal character, which
// may be \0 at the end of the string.
static inline uint32_t fast_strtoul10(const char *str, const char **endptr) {
	uint32_t result = 0;
	unsigned char c;
	for (; (c = *str - '0') <= 9; str++)
		result = result * 10 + c;
	if (endptr)
		*endptr = str;
	return result;
}

// Same as fast_strtoul10, but for hex strings.
static inline uint32_t fast_strtoul16(const char *str, const char **endptr) {
	uint32_t result = 0;
	unsigned char c;
	while ((c = *str - '0') <= 9 // 0-9
				 || ((c -= 7) >= 10 && c <= 15) // A-F
				 || ((c -= 32) >= 10 && c <= 15)) { // a-f
		result = result * 16 + c;
		str++;
	}
	if (endptr)
		*endptr = str;
	return result;
}
// end shamelessly ripped from pixelnuke

static void net_send(px_client_t * client, char * str) {
	send(client->socket, str, strlen(str), 0);
	send(client->socket, "\n", 1, 0);
}

static void net_err(px_client_t * client, char * str) {
	send(client->socket, "ERROR: ", 7, 0);
	send(client->socket, str, strlen(str), 0);
	send(client->socket, "\n", 1, 0);
}

static void poke_main_thread(void) {
	char ** argv = malloc(sizeof(char*));
	if (argv) {
		*argv = strdup("--start");
		if (*argv) {
			timer_add(0, px_moduleno, 1, argv);
			wait_until_break();
		} else {
			free(argv);
		}
	}
}

// returns 0 on successful line read
static int px_client_readln(px_client_t * client, char * line, size_t maxread) {
	char * line_read = line;
	do {
		ssize_t r = read(client->socket, line_read, 1);
		if (r <= 0) return -1;

		line_read++;
		maxread--;
	} while (maxread != 0 && *(line_read-1) != '\n');
	return 0;
}

// Returns true to remove the client.
static int px_client_update(px_client_t * client) {
	char line[20];
	if (px_client_readln(client, line, sizeof(line))) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			// Okay!
			return 0;
		}
		return 1;
	} else { // parse line
		// ripped from pixelnuke
		if (line[0] == 'P' && line[1] == 'X') {
			const char * ptr = line + 3;
			const char * endptr = ptr;
			errno = 0;

			uint32_t x = fast_strtoul10(ptr, &endptr);
			if (endptr == ptr) {
				net_err(client, "ERROR: Invalid command (expected decimal as first parameter)");
				return 1;
			}
			if (*endptr == '\n') {
				net_err(client, "ERROR: Invalid command (second parameter required)");
				return 1;
			}

			endptr++; // eat space (or whatever non-decimal is found here)

			uint32_t y = fast_strtoul10((ptr = endptr), &endptr);
			if (endptr == ptr) {
				net_err(client, "Invalid command (expected decimal as second parameter)");
				return 1;
			}

			// PX <x> <y> -> Get RGB color at position (x,y) or '0x000000' for out-of-range queries
			if (*endptr == '\n') {
				char str[64];

				uint32_t pixel = *(uint32_t *)&px_array[((x + client->off_x)*px_mx + (y + client->off_y)) * 3];
				sprintf(str, "PX %u %u %06X", x, y, pixel & 0xFFFFFF);
				net_send(client, str);
				poke_main_thread();
				return 0;
			}

			endptr++; // eat space (or whatever non-decimal is found here)

			// PX <x> <y> BB|RRGGBB|RRGGBBAA
			uint32_t c = fast_strtoul16((ptr = endptr), &endptr);
			if (endptr == ptr) {
				net_err(client,
								"Third parameter missing or invalid (should be hex color)");
				return 1;
			}

			if (endptr - ptr == 6) {
				// done
			} else if (endptr - ptr == 8) {
				// RGBA -> RGB (we dont do alpha)
				c = c >> 8;
			} else if (endptr - ptr == 2) {
				// WW -> RGBA // UNTESTED, THIS WILL BREAK
				c = (c << 24) + (c << 16) + (c << 8) + 0xff;
			} else {
				net_err(client,
								"Color hex code must be 2, 6 or 8 characters long (WW, RGB or RGBA)");
				return 1;
			}

			px_pixelcount++;
			px_array[((x + client->off_x)*px_mx + (y + client->off_y)) * 3]    = c >> 16;
			px_array[((x + client->off_x)*px_mx + (y + client->off_y)) * 3 +1] = c >>  8;
			px_array[((x + client->off_x)*px_mx + (y + client->off_y)) * 3 +2] = c;
		} else if (fast_str_startswith("OFFSET", line)) {
			const char * ptr = line + 7;
			const char * endptr = ptr;
			errno = 0;

			uint32_t x = fast_strtoul10(ptr, &endptr);
			if (endptr == ptr) {
				net_err(client, "ERROR: Invalid command (expected decimal as first parameter)");
				return 1;
			}
			if (*endptr == '\n') {
				net_err(client, "ERROR: Invalid command (second parameter required)");
				return 1;
			}

			endptr++; // eat space (or whatever non-decimal is found here)

			uint32_t y = fast_strtoul10((ptr = endptr), &endptr);
			if (endptr == ptr) {
				net_err(client, "Invalid command (expected decimal as second parameter)");
				return 1;
			}

			// OFFSET <x> <y> -> set offset to position (x,y) for future use
			if (*endptr == '\n') {
				client->off_x = x;
				client->off_y = y;
				poke_main_thread();
				return 0;
			}
		} else if (fast_str_startswith("SIZE", line)) {
			char str[64];
			snprintf(str, 64, "SIZE %d %d", px_mx, px_my);
			net_send(client, str);
		} else if (fast_str_startswith("STATS", line)) {
			char str[128];
			snprintf(str, 128, "STATS px:%u conn:%u", px_pixelcount,
							 px_clientcount);
			net_send(client, str);
		} else if (fast_str_startswith("HELP", line)) {
			net_send(client,
							 "\
PX x y: Get color at position (x,y)\n\
PX x y rrggbb(aa): Draw a pixel (alpha ignored)\n\
SIZE: Get canvas size\n\
STATS: Return statistics");

		} else {
			net_err(client, "Unknown command");
			return 1;
		}
		// END ripped from pixelnuke
	}
	poke_main_thread();

	return 0;
}

// Allows or closes the socket.
static int px_client_new(px_client_t ** list, int sock) {
	px_client_t * c = malloc(sizeof(px_client_t));
	if (!c) {
		close(sock);
		return 0;
	}
	c->socket = sock;
	c->off_x = 0;
	c->off_y = 0;
	c->next = NULL;
	if (*list)
		c->next = *list;
	*list = c;
	return 1;
}

// Makes an FD nonblocking
static void px_nbs(int sock) {
	int flags = fcntl(sock, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
}

static void * px_thread_func(void * n) {
	px_client_t * list = 0;
	int server;
	struct sockaddr_in sa_bpwr;
	server = socket(AF_INET, SOCK_STREAM, 0); // It's either 0 or 6...
	if (server < 0) {
		fputs("error creating socket! -- Pixelflut\n", stderr);
		return 0;
	}
	// more magic
	memset(&sa_bpwr, 0, sizeof(sa_bpwr));
	sa_bpwr.sin_family = AF_INET;
	sa_bpwr.sin_port = htons(PX_PORT);
	sa_bpwr.sin_addr.s_addr = INADDR_ANY;

	// prepare server...
	if (bind(server, (struct sockaddr *) &sa_bpwr, sizeof(sa_bpwr))) {
		fputs("error binding socket! -- Pixelflut\n", stderr);
		return 0;
	}
	if (listen(server, 32)) {
		fputs("error finalizing socket! -- Pixelflut\n", stderr);
		return 0;
	}
	px_nbs(server);
	px_nbs(px_shutdown_fd_ot);
	// --
	fd_set rset;
	char sdbuf;
	while (read(px_shutdown_fd_ot, &sdbuf, 1) <= 0) {
		// Accept?
		int accepted = accept(server, NULL, NULL);
		if (accepted >= 0) {
			px_nbs(accepted);
			if (px_client_new(&list, accepted))
				FD_SET(accepted, &rset);
		}
		// select zeroes FDs >:(
		FD_ZERO(&rset);
		// Go through all clients!
		px_client_t ** backptr = &list;
		while (*backptr) {
			if (px_client_update(*backptr)) {
				void * on = (*backptr)->next;
				close((*backptr)->socket);
				free(*backptr);
				*backptr = on;
			} else {
				FD_SET((*backptr)->socket, &rset);
				backptr = (px_client_t**) &((*backptr)->next);
			}
		}
		FD_SET(px_shutdown_fd_ot, &rset);
		FD_SET(server, &rset);
		select(FD_SETSIZE, &rset, NULL, NULL, NULL);
	}
	// Close & Deallocate
	while (list) {
		px_client_t * nxt = (px_client_t*) list->next;
		close(list->socket);
		free(list);
		list = nxt;
	}
	close(server);
	return 0;
}

int init(int moduleno, char* argstr) {
	px_mtcountdown = 100; // frames
	// Shutdown signalling pipe
	int tmp[2];
	if (pipe(tmp) != 0)
		return 1;

	px_mx = matrix_getx();
	px_my = matrix_gety();
	// For whatever reason, the *receiver* is FD 0.
	px_shutdown_fd_mt = tmp[1];
	px_shutdown_fd_ot = tmp[0];
	px_moduleno = moduleno;
	px_task = oscore_task_create("bgm_pixelflut", px_thread_func, NULL);

	return 0;
}

int draw(int argc, char ** argv) {
	if (argc) {
		px_mtcountdown = PX_MTCOUNTDOWN_MAX;
		px_mtlastframe = udate();
	}
	matrix_clear();
	int indx = 0;
	// Note the use of 65535 as the maximum amount (making 65534 the maximum index),
	//  this aligns to the magic number 3.
	int w = px_mx, h = matrix_gety();
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			byte r = px_array[indx++];
			byte g = px_array[indx++];
			byte b = px_array[indx++];
			RGB rgb = RGB(r, g, b);
#ifdef PX_SNAKE
			matrix_set(i, (i & 1) ? j : h - (j + 1), &rgb);
#else
			matrix_set(i, j, &rgb);
#endif
			if (indx == 65535)
				break;
		}
		if (indx == 65535)
			break;
	}
	matrix_render();
	if ((--px_mtcountdown) > 0) {
		timer_add(px_mtlastframe += FRAMETIME, px_moduleno, 0, NULL);
		return 0;
	}
	return 1;
}

void reset(void) {
	// Nothing?
}

int deinit() {
	char blah = 0;
	if (write(px_shutdown_fd_mt, &blah, 1) != -1)
		oscore_task_join(px_task);
	close(px_shutdown_fd_mt);
	close(px_shutdown_fd_ot);
	return 0;
}
