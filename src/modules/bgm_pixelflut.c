// PIXELFLUT: Pixelflut output
// This allows using SLED as a Pixelflut output.
// Should a Pixelflut connection connect,
//  the module will start.
// Should all Pixelflut connections disconnect,
//  the module will end.

#ifdef __linux__
#define _GNU_SOURCE
#endif

#ifdef __APPLE__
#define MSG_NOSIGNAL SO_NOSIGPIPE
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
#include "mod.h"
#include "asl.h"
#include "taskpool.h"

// 😥
static RGB * px_array;

static int px_shutdown_fd_mt, px_shutdown_fd_ot;
// px_mtcountdown is the time until we decide to end. It's main-thread-only.
static int px_moduleno, px_mtcountdown;
static unsigned int px_pixelcount, px_clientcount;

// This is MT only as of some commit or another.
// Ignored by netthreads because even if they put stuff on the matrix at the wrong time,
//  it'll then instantly get overwritten by the MT switching to pixelflut
static int px_bgminactive;

static int px_mx, px_my;
static ulong px_mtlastframe;
static oscore_task px_task;


#define FPS 60
// #define PX_MTCOUNTDOWN_MAX 120
#define FRAMETIME (T_SECOND / FPS)
#define PX_PORT 1337
// The maximum, including 0, size of a line.
#define PX_LINESIZE 0x2000

typedef struct {
	int socket;
	size_t linelen;
	char line[PX_LINESIZE];
} px_buffer_t;

typedef struct {
	int socket; // The socket
	void * next; // The next client
	px_buffer_t * buffer; // The current line buffer.
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

static void net_send(px_buffer_t * client, char * str) {
	size_t len = strlen(str);
	str[len] = '\n';
	send(client->socket, str, len + 1, MSG_NOSIGNAL);
}

static void net_err(px_buffer_t * client, char * str) {
	send(client->socket, "ERROR: ", 7, MSG_NOSIGNAL);
	send(client->socket, str, strlen(str), MSG_NOSIGNAL);
	send(client->socket, "\n", 1, MSG_NOSIGNAL);
}

static void poke_main_thread(void) {
	timer_add(0, px_moduleno, 1, NULL);
	wait_until_break();
}

// Executes the line given.
// Please ignore the return value.
// I have a sneaking suspicion the returns were in the original code.
// Anyway, as this uses matrix_set, and thus checks if the BGM is active/inactive, make sure to use this inside the BGM activity lock.
static int px_buffer_executeline(const char * line, px_buffer_t * client) {
	// In the original version, this was ripped from pixelnuke,
	//  and it remains that way, but hopefully I've changed it enough.
	if (line[0] == 'P' && line[1] == 'X') {
		const char * ptr = line + 3;
		const char * endptr = ptr;

		uint32_t x = fast_strtoul10(ptr, &endptr);
		if (endptr == ptr) {
			net_err(client, "ERROR: Invalid command (expected decimal as first parameter)");
			return 1;
		}
		if (!*endptr) {
			net_err(client, "ERROR: Invalid command (second parameter required)");
			return 1;
		}

		endptr++; // eat space (or whatever non-decimal is found here)

		uint32_t y = fast_strtoul10((ptr = endptr), &endptr);
		if (endptr == ptr) {
			net_err(client, "Invalid command (expected decimal as second parameter)");
			return 1;
		}

		int inbounds = (x < px_mx) && (y < px_my);

		size_t index = x + (y * px_mx);

		// PX <x> <y> -> Get RGB color at position (x,y) or '0x000000' for out-of-range queries
		if (!*endptr) {
			char str[64];

			RGB pixel = RGB(0, 0, 0);
			if (inbounds)
				pixel = px_array[index];
			sprintf(str, "PX %u %u %02X%02X%02X", x, y, pixel.red, pixel.green, pixel.blue);
			net_send(client, str);
			return 0;
		}

		endptr++; // eat space (or whatever non-decimal is found here)

		// PX <x> <y> BB|RRGGBB|RRGGBBAA
		uint32_t c = fast_strtoul16((ptr = endptr), &endptr);
		if (endptr == ptr) {
			net_err(client, "Third parameter missing or invalid (should be hex color)");
			return 1;
		}

		RGB pixel;
		if (endptr - ptr == 6) {
			// 0x00RRGGBB
			pixel.red = c >> 16;
			pixel.green = c >> 8;
			pixel.blue = c;
		} else if (endptr - ptr == 8) {
			// 0xRRGGBBAA
			pixel.red = c >> 24;
			pixel.green = c >> 16;
			pixel.blue = c >> 8;
		} else if (endptr - ptr == 2) {
			// 0x000000Gr
			pixel.red = c;
			pixel.green = c;
			pixel.blue = c;
		} else {
			net_err(client, "Color hex code must be 2, 6 or 8 characters long (WW, RGB or RGBA)");
			return 1;
		}

		px_pixelcount++;

		if (!inbounds)
			return 0;

		matrix_set(x, y, pixel);
		px_array[index] = pixel;
	/*} else if (fast_str_startswith("OFFSET", line)) {
		const char * ptr = line + 7;
		const char * endptr = ptr;

		uint32_t x = fast_strtoul10(ptr, &endptr);
		if (endptr == ptr) {
			net_err(client, "ERROR: Invalid command (expected decimal as first parameter");
		} */
	} else if (fast_str_startswith("SIZE", line)) {
		char str[64];
		snprintf(str, 64, "SIZE %d %d", px_mx, px_my);
		net_send(client, str);
	} else if (fast_str_startswith("STATS", line)) {
		char str[128];
		snprintf(str, 128, "STATS px:%u conn:%u", px_pixelcount, px_clientcount);
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
	return 0;
}

static void px_buffer_update(void * buf) {
	px_buffer_t * buffer = buf;
	char * line = buffer->line;
	while (1) {
		char * ch = strchr(line, '\n');
		if (ch)
			*ch = 0;
		px_buffer_executeline(line, buf);
		if (!ch)
			break;
		line = ch + 1;
	}
	free(buffer);
	poke_main_thread();
}

// Returns true to remove the client.
static int px_client_update(px_client_t * client) {
	px_buffer_t * cbuf = client->buffer;
	ssize_t addlen = read(client->socket, cbuf->line + cbuf->linelen, PX_LINESIZE - (1 + cbuf->linelen));
	if (addlen < 0) {
		// All errors except these are assumed to mean the connection died.
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 0;
		return 1;
	} else if (addlen == 0) {
		// End Of File -> socket closed
		return 1;
	} else {
		// Zero-terminate the resulting string.
		cbuf->linelen += addlen;
		cbuf->line[cbuf->linelen] = 0;
	}
	char * chp = memrchr(cbuf->line, '\n', cbuf->linelen);
	if (chp) {
		// Create new buffer, put the remainder in it
		px_buffer_t * nb = malloc(sizeof(px_buffer_t));
		if (!nb)
			return 1;
		nb->linelen = (cbuf->line + cbuf->linelen) - (chp + 1);
		memcpy(nb->line, chp + 1, nb->linelen + 1);
		nb->socket = client->socket;
		// The new remainder buffer belongs to us...
		client->buffer = nb;
		// The old line buffer is sent to the taskpool
		*chp = 0;
		taskpool_submit(TP_GLOBAL, px_buffer_update, cbuf);
	}
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
	c->next = NULL;
	c->buffer = malloc(sizeof(px_buffer_t));
	if (!c->buffer) {
		free(c);
		close(sock);
		return 0;
	}
	c->buffer->socket = sock;
	c->buffer->line[0] = 0;
	c->buffer->linelen = 0;
	if (*list)
		c->next = *list;
	*list = c;
	px_clientcount++;
	return 1;
}

// Makes an FD nonblocking
static void px_nbs(int sock) {
	int flags = fcntl(sock, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
}

// temporary..
#define PIXELFLUT_USE_SELECT 1
#ifdef PIXELFLUT_USE_SELECT
static void * px_thread_func(void * n) {
	px_client_t * list = 0;
	int server;
	struct sockaddr_in sa_bpwr;
	server = socket(AF_INET, SOCK_STREAM, 0); // It's either 0 or 6...
	if (server < 0) {
		fputs("error creating socket! -- Pixelflut\n", stderr);
		return NULL;
	}
	// more magic
	memset(&sa_bpwr, 0, sizeof(sa_bpwr));
	sa_bpwr.sin_family = AF_INET;
	sa_bpwr.sin_port = htons(PX_PORT);
	sa_bpwr.sin_addr.s_addr = INADDR_ANY;

	// prepare server...
	if (bind(server, (struct sockaddr *) &sa_bpwr, sizeof(sa_bpwr))) {
		fputs("error binding socket! -- Pixelflut\n", stderr);
		return NULL;
	}
	if (listen(server, 32)) {
		fputs("error finalizing socket! -- Pixelflut\n", stderr);
		return NULL;
	}
	px_nbs(server);
	px_nbs(px_shutdown_fd_ot);
	// --
	fd_set rset, active_fds;
	char sdbuf;

	FD_ZERO(&active_fds);
	FD_SET(px_shutdown_fd_ot, &active_fds);
	FD_SET(server, &active_fds);

	while (1) {
		rset = active_fds;
		select(FD_SETSIZE, &rset, NULL, NULL, NULL);

		if(FD_ISSET(px_shutdown_fd_ot, &rset) && (read(px_shutdown_fd_ot, &sdbuf, 1) <= 0)) {
			break;
		}

		// Accept?
		if(FD_ISSET(server, &rset)) {
			int accepted = accept(server, NULL, NULL);
			if (accepted >= 0) {
				px_nbs(accepted);
				px_client_new(&list, accepted);
				FD_SET(accepted, &active_fds);
			}
		}

		// Go through all clients, holding the BGMI lock so that the status of "are we in control of the matrix" cannot change.
		px_client_t ** backptr = &list;
		while (*backptr) {
			if (FD_ISSET((*backptr)->socket, &rset)) {
				if(px_client_update(*backptr)) {
					void *on = (*backptr)->next;
					// we don't want to close the socket until all taskpool threads are done
					taskpool_wait(TP_GLOBAL);
					close((*backptr)->socket);
					FD_CLR((*backptr)->socket, &active_fds);
					free((*backptr)->buffer);
					free(*backptr);
					*backptr = on;
					px_clientcount--;
				}
				else {
					backptr = (px_client_t**) &((*backptr)->next);
				}
			}
			else {
				backptr = (px_client_t**) &((*backptr)->next);
			}
		}
	}
	// we don't want to close the socket until all taskpool threads are done
	taskpool_wait(TP_GLOBAL);
	// Close & Deallocate
	while (list) {
		px_client_t * nxt = (px_client_t*) list->next;
		free(list->buffer);
		close(list->socket);
		free(list);
		list = nxt;
	}
	close(server);
	return NULL;
}
#endif

int init(int moduleno, char* argstr) {
	px_mtcountdown = FPS; // frames
	// Shutdown signalling pipe
	int tmp[2];
	if (pipe(tmp) != 0)
		return 1;

	px_mx = matrix_getx();
	px_my = matrix_gety();
	px_array = calloc(px_mx * px_my, sizeof(RGB));
	if (!px_array) {
		// Insufficient RAM.
		close(tmp[0]);
		close(tmp[1]);
		return 1;
	}
	// For whatever reason, the *receiver* is FD 0.
	px_shutdown_fd_mt = tmp[1];
	px_shutdown_fd_ot = tmp[0];
	px_moduleno = moduleno;
	px_bgminactive = 1;
	px_task = oscore_task_create("bgm_pixelflut", px_thread_func, NULL);

	return 0;
}

int draw(int argc, char ** argv) {
	if (argc) {
		if (argv)
			free(argv);
#ifdef PX_MTCOUNTDOWN_MAX
		px_mtcountdown = PX_MTCOUNTDOWN_MAX;
#endif
		px_mtlastframe = udate();
		if (px_bgminactive) {
			int indx = 0;
			for (int j = 0; j < px_my; j++) {
				for (int i = 0; i < px_mx; i++) {
					matrix_set(i, j, px_array[indx]);
					indx++;
				}
			}
			px_bgminactive = 0;
		}
	}
	// We still do matrix_render on the main thread.
	// Mainly because not doing so would be a very quick road to blocking up the network thread.
	matrix_render();
#ifdef PX_MTCOUNTDOWN_MAX
	if ((--px_mtcountdown) > 0) {
#endif
		timer_add(px_mtlastframe += FRAMETIME, px_moduleno, 0, NULL);
		return 0;
#ifdef PX_MTCOUNTDOWN_MAX
	}
	px_bgminactive = 1;
	return 1;
#endif
}

void reset(void) {
	// If it's 0, then quite a mess will result...
	px_bgminactive = 1;
}

int deinit() {
	char blah = 0;
	if (write(px_shutdown_fd_mt, &blah, 1) != -1)
		oscore_task_join(px_task);
	close(px_shutdown_fd_mt);
	close(px_shutdown_fd_ot);
	free(px_array);
	return 0;
}
