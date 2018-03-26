// OPC: Open Pixel Control output
// This allows using SLED as an OPC output.
// Shoudl an OPC connection connect,
//  the module will start.
// Should all OPC connections disconnect,
//  the module will end.

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "timers.h"
#include "matrix.h"
#include "main.h"
#include "modloader.h"
#include "asl.h"

// It is *IMPOSSIBLE* for an OPC client to send a length that escapes this buffer.
byte opc_array[65536];
// Where data goes that we ignore
byte opc_scratch_array[65536];

int opc_shutdown_fd_mt, opc_shutdown_fd_ot, opc_shutdown_flag;
// opc_mtcountdown is the time until we decide to end. It's main-thread-only.
int opc_moduleno, opc_mtcountdown;
ulong opc_mtlastframe;
#define OPC_MTCOUNTDOWN_MAX 100
#define FRAMETIME 10000
#define OPC_SNAKE
pthread_t opc_thread;

typedef struct {
	byte channel;
	byte command;
	byte len_h, len_l;
} opc_headbuffer;

typedef struct {
	int socket; // The socket
	// Flags for state machine
	int header; // Starts at 1.
	byte * position; // Starts at buf.
	size_t position_remain; // Starts at the size of buf.
	opc_headbuffer buf; // Initialization here does not matter
	void * next; // The next client
} opc_client_t;

// Returns true to remove the client.
int opc_client_update(opc_client_t * client) {
	// Read
	ssize_t r = read(client->socket, client->position, client->position_remain);
	if (r < 0) {
		// Error!
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			// Okay!
			return 0;
		}
		return 1;
	} else {
		// Advance
		client->position += r;
		client->position_remain -= r;
		if (!client->position_remain) {
			if (client->header) {
				// Set things up to read the data.
				int valid = (client->buf.channel <= 1) && (client->buf.command == 0);
				client->header = 0;
				client->position = valid ? opc_array : opc_scratch_array;
				client->position_remain = (((size_t) (client->buf.len_h)) << 8) | (client->buf.len_l);
			} else {
				// Please render now.
				// We need to tell the module that it's the first frame,
				//  so main thread has control of the counter.
				// How do we do this? MORE MALLOCs, of course!
				char ** argv = malloc(sizeof(char*));
				if (argv) {
					*argv = strdup("--start");
					if (*argv) {
						timer_add(0, opc_moduleno, 1, argv);
					} else {
						free(argv);
					}
				}
				client->header = 1;
				client->position = (byte *) &(client->buf);
				client->position_remain = sizeof(opc_headbuffer);
			}
		}
	}
	return 0;
}

// Allows or closes the socket.
int opc_client_new(opc_client_t ** list, int sock) {
	opc_client_t * c = malloc(sizeof(opc_client_t));
	if (!c) {
		close(sock);
		return 0;
	}
	c->socket = sock;
	c->header = 1;
	c->position = (byte *) &(c->buf);
	c->position_remain = sizeof(opc_headbuffer);
	c->next = NULL;
	if (*list)
		c->next = *list;
	*list = c;
	return 1;
}

// Makes an FD nonblocking
void opc_nbs(int sock) {
	int flags = fcntl(sock, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
}

void * opc_thread_func(void * n) {
	opc_client_t * list = 0;
	int server;
	struct sockaddr_in sa_bpwr;
	server = socket(AF_INET, SOCK_STREAM, 0); // It's either 0 or 6...
	if (server < 0) {
		fputs("error creating socket! -- OPC\n", stderr);
		return 0;
	}
	// more magic
	memset(&sa_bpwr, 0, sizeof(sa_bpwr));
	sa_bpwr.sin_family = AF_INET;
	sa_bpwr.sin_port = htons(7890);
	sa_bpwr.sin_addr.s_addr = INADDR_ANY;
	// prepare server...
	if (bind(server, (struct sockaddr *) &sa_bpwr, sizeof(sa_bpwr))) {
		fputs("error binding socket! -- OPC\n", stderr);
		return 0;
	}
	if (listen(server, 32)) {
		fputs("error finalizing socket! -- OPC\n", stderr);
		return 0;
	}
	opc_nbs(server);
	// --
	fd_set nset, rset;
	FD_ZERO(&nset);
	FD_ZERO(&rset);
	FD_SET(opc_shutdown_fd_ot, &rset);
	FD_SET(server, &rset);
	while (!opc_shutdown_flag) {
		// Accept?
		int accepted = accept(server, NULL, NULL);
		if (accepted >= 0) {
			opc_nbs(accepted);
			if (opc_client_new(&list, accepted))
				FD_SET(accepted, &rset);
		}
		// Go through all clients!
		opc_client_t ** backptr = &list;
		while (*backptr) {
			if (opc_client_update(*backptr)) {
				void * on = (*backptr)->next;
				FD_CLR((*backptr)->socket, &rset);
				close((*backptr)->socket);
				free(*backptr);
				*backptr = on;
			} else {
				backptr = (opc_client_t**) &((*backptr)->next);
			}
		}
		select(FD_SETSIZE, &rset, &nset, &nset, NULL);
	}
	// Close & Deallocate
	while (list) {
		opc_client_t * nxt = (opc_client_t*) list->next;
		close(list->socket);
		free(list);
		list = nxt;
	}
	close(server);
	return 0;
}

int init(int moduleno) {
	opc_shutdown_flag = 0;
	opc_mtcountdown = 100;
	// Shutdown signalling pipe
	int tmp[2];
	pipe(tmp);
	opc_shutdown_fd_mt = tmp[0];
	opc_shutdown_fd_ot = tmp[1];
	opc_moduleno = moduleno;
	pthread_create(&opc_thread, NULL, opc_thread_func, NULL);
	return 0;
}

int draw(int argc, char ** argv) {
	if (argc) {
		opc_mtcountdown = OPC_MTCOUNTDOWN_MAX;
		opc_mtlastframe = udate();
	}
	matrix_clear();
	int indx = 0;
	// Note the use of 65535 as the limit,
	//  this aligns to the magic number 3.
	int w = matrix_getx(), h = matrix_gety();
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			byte r = opc_array[indx++];
			byte g = opc_array[indx++];
			byte b = opc_array[indx++];
			RGB rgb = RGB(r, g, b);
#ifdef OPC_SNAKE
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
	if ((--opc_mtcountdown) > 0) {
		timer_add(opc_mtlastframe += FRAMETIME, opc_moduleno, 0, NULL);
		return 0;
	}
	return 1;
}

int deinit() {
	opc_shutdown_flag = 1;
	write(opc_shutdown_fd_mt, &opc_shutdown_flag, sizeof(opc_shutdown_flag));
	pthread_join(opc_thread, NULL);
    close(opc_shutdown_fd_mt);
    close(opc_shutdown_fd_ot);
	return 0;
}
