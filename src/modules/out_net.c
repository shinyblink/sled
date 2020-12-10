
// Output pixel data stream over TCP or Unix socket.
// This output module is originally based on out_udp.
//
// Copyright (c) 2019, Adrian "vifino" Pistol <vifino@tty.sh>
// Copyright (c) 2020, Piotr Esden-Tempski <piotr@esden.net>
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
#include <timers.h>
#include <stdlib.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <util.h>
#include <assert.h>

#define TILE_PLAIN 1
#define TILE_SNAKE 2

#define NET_TCP 1
#define NET_UDP 2
#define NET_SOCK 3

static int sock = -1;
struct sockaddr_in sio_inet;
struct sockaddr_un sio_unix;
static int port;
static int X_SIZE;
static int Y_SIZE;
static int tiletype;
static int nettype;

#define NUMPIX (X_SIZE * Y_SIZE)

#define PARAM_ERR_STR "out_net argstring %s. \n" \
"For reference these are valid examples:\n" \
"A TCP server with a 16x8 resolution matrix with snake layout.\n" \
" -o net:tcp:192.168.69.42:1234,16x8,snake\n" \
"A UDP server with a 10x10 resolution matrix with plain layout.\n" \
" -o net:udp:192.168.69.42:1234,10x10,plain\n" \
"A UNIX socket server with a 32x32 resolution matrix and snake layout\n" \
" -o net:socket:/tmp/rgb_matrix.sock,32x32,snake\n"

// Message will be:
// <R,G,B bytes..>
// of raw values to fill a frame
static byte* message;

int init (int moduleno, char* argstr) {
	// Parse parameter string
	if (argstr == NULL) {
		eprintf(PARAM_ERR_STR, "not set");
		return 3;
	}

	char* param_str = argstr; // Initialize the pointer we will be walking over to split the string with.
	char* net_type_str; // This can be one of tcp, udp or socket
	char* ip_sock_str; // either the ip address or socket path
	char* port_str; // if ip or udp this will contain port number
	char* tile_type_str; // select pixel tiling, normal or snake
	char* x_str; // matrix width
	char* y_str; // matrix height

	// TCP:aaa.aaa.aaa.aaa:pppp,wwxhh,snake
	// UDP:aaa.aaa.aaa.aaa:pppp,wwxhh,snake
	// SOCK:/path/path/name.sock,wwxhh,snake
	if ((net_type_str = strsep(&param_str, ":")) == NULL) {
		eprintf(PARAM_ERR_STR, "doesn't contain a type seperator");
		return 3;
	}

	nettype = -1;
	if (strcmp(net_type_str, "tcp") == 0) nettype = NET_TCP;
	if (strcmp(net_type_str, "udp") == 0) nettype = NET_UDP;
	if (strcmp(net_type_str, "socket") == 0) nettype = NET_SOCK;
	if (nettype == -1) {
		eprintf(PARAM_ERR_STR, "doesn't contain a valid network type");
		return 3;
	}

	// tcp:AAA.AAA.AAA.AAA:pppp,wwxhh,snake
	// udp:AAA.AAA.AAA.AAA:pppp,wwxhh,snake
	// sock:/PATH/PATH/NAME.SOCK,wwxhh,snake
	if ((ip_sock_str = strsep(&param_str, nettype == NET_SOCK ? "," : ":")) == NULL) {
		eprintf(PARAM_ERR_STR, "doesn't contain a socket/ip separator");
		return 3;
	}

	if (nettype != NET_SOCK) {
		// tcp:aaa.aaa.aaa.aaa:PPPP,wwxhh,snake
		// udp:aaa.aaa.aaa.aaa:PPPP,wwxhh,snake
		if ((port_str = strsep(&param_str, ",")) == NULL) {
			eprintf(PARAM_ERR_STR, "doesn't contain port");
			return 3;
		}
	} else {
		port_str = "";
	}

	// tcp:aaa.aaa.aaa.aaa:pppp,WWxhh,snake
	// udp:aaa.aaa.aaa.aaa:pppp,WWxhh,snake
	// sock:/path/path/name.sock,WWxhh,snake
	if ((x_str = strsep(&param_str, "x")) == NULL) {
		eprintf(PARAM_ERR_STR, "doesn't contain X size");
		return 3;
	}

	X_SIZE = util_parse_int(x_str);
	if (X_SIZE == 0) {
		eprintf(PARAM_ERR_STR, "doesn't contain a X matrix size");
		return 4;
	}

	// tcp:aaa.aaa.aaa.aaa:pppp,wwxHH,snake
	// udp:aaa.aaa.aaa.aaa:pppp,wwxHH,snake
	// sock:/path/path/name.sock,wwxHH,snake
	if ((y_str = strsep(&param_str, ",")) == NULL) {
		eprintf(PARAM_ERR_STR, "doesn't contain Y size");
		return 3;
	}

	Y_SIZE = util_parse_int(y_str);
	if (Y_SIZE == 0) {
		eprintf(PARAM_ERR_STR, "doesn't contain a Y matrix size");
		return 4;
	}

	// tcp:aaa.aaa.aaa.aaa:pppp,wwxhh,SNAKE
	// udp:aaa.aaa.aaa.aaa:pppp,wwxhh,SNAKE
	// sock:/path/path/name.sock,wwxhh,SNAKE
	tile_type_str = param_str;
	tiletype = -1;
	if (strcmp(tile_type_str, "plain") == 0) tiletype = TILE_PLAIN;
	if (strcmp(tile_type_str, "snake") == 0) tiletype = TILE_SNAKE;
	if (tiletype == -1) {
		eprintf(PARAM_ERR_STR, "doesn't contain a valid tiling type");
		return 4;
	}

	// Create socket
	switch (nettype) {
		case NET_TCP:
			sock=socket(AF_INET, SOCK_STREAM, 0);
			break;
		case NET_UDP:
			sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			break;
		case NET_SOCK:
			sock=socket(AF_UNIX, SOCK_STREAM, 0);
			break;
	}

	if (sock == -1) {
		perror("out_net: Failed to initialize socket");
		return 2;
	}

	// TCP and UDP related socket initialization
	if (nettype != NET_SOCK) {
		memset((char *) &sio_inet, 0, sizeof(struct sockaddr_in));
		sio_inet.sin_family = AF_INET;

		// Parse IP address
		if (inet_aton(ip_sock_str, &sio_inet.sin_addr) == 0) {
			eprintf(PARAM_ERR_STR, "doesn't contain a valid IP");
			return 3;
		}

		// Parse port number
		port = util_parse_int(port_str);
		if (port == 0) {
			eprintf(PARAM_ERR_STR, "doesn't contain a valid port number");
			return 3;
		}
		sio_inet.sin_port = htons(port);

		// Connecting to the server socket
		if (connect(sock, (struct sockaddr*)&sio_inet, sizeof(sio_inet)) != 0) {
			eprintf("out_net failed to connect to server.");
			return 5;
		}

		// This migt be a necessary hack if the OS decides to buffer data
		// Mind you according to the internet this is not a reliable solution.
		// The OS might or might not respect this setting.
		if (nettype == NET_TCP) {
			int flag = 1;
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
		}

	// Open a Unix socket
	} else {
		memset((char *) &sio_unix, 0, sizeof(struct sockaddr_un));
		sio_unix.sun_family = AF_UNIX;
		strncpy(sio_unix.sun_path, ip_sock_str, strnlen(ip_sock_str, 107));

		// Connecting to the server socket
		if (connect(sock, (struct sockaddr*)&sio_unix, sizeof(sio_unix)) != 0) {
			eprintf("out_net failed to connect to server.");
			return 5;
		}
	}

	// Allocate the message buffer.
	message = calloc(NUMPIX * 3, 1);
	assert(message); // 2lazy to handle it properly.

	// Free stuff.
	free(argstr);

	return 0;
}

int getx(int _modno) {
	return X_SIZE;
}
int gety(int _modno) {
	return Y_SIZE;
}

// Pixel position calculation used only locally within the module
static inline int ppos(int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);

	switch (tiletype) {
	case TILE_PLAIN:
		return (x + (y * X_SIZE));
		break;
	case TILE_SNAKE:
		return (((y % 2) == 0 ? x : (X_SIZE - 1) - x) + X_SIZE * y);
		break;
	}
	return -1;
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);

	int pos = ppos(x, y) * 3;
	message[pos + 0] = color.red;
	message[pos + 1] = color.green;
	message[pos + 2] = color.blue;
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);

	int pos = ppos(x, y) * 3;
	return RGB(message[pos + 0], message[pos + 1], message[pos + 2]);
}

int clear(int _modno) {
	memset(&message[0], '\0', NUMPIX * 3);
	return 0;
};

int render(void) {
	
	// send frame data packets
	if (send(sock, message, NUMPIX * 3, 0) == -1) {
		perror("out_net: Failed to send frame data packet");
		return 5;
	}

	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	return timers_wait_until_break_core();
}

void deinit(int _modno) {
	close(sock);
	free(message);
}
