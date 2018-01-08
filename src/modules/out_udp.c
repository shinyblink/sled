// UDP output.
// Follows the protocol of CalcProgrammer1/KeyboardVisualizer's LED strip code.
// Quite a big mess. It works, however.

#include <types.h>
#include <timers.h>
#include <stdlib.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <util.h>
#include <assert.h>

#define BUFLEN 1024
#define TILE_PLAIN 1
#define TILE_SNAKE 2

static int sock = -1;
struct sockaddr_in sio;
static int port;
static int X_SIZE;
static int Y_SIZE;
static int tiletype;
static char* envdup;

#define NUMPIX (X_SIZE * Y_SIZE)

// Message will be:
// 0xAA <R,G,B bytes..> <2 bytes checksum, unsigned short, hi, low>
byte* message;

int init(void) {
	// Partially initialize the socket.
	if ((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("out_udp: Failed to initialize socket");
		return 2;
	}
	memset((char *) &sio, 0, sizeof(struct sockaddr_in));
	sio.sin_family = AF_INET;

	// Parse env. This sucks.
	char* var = getenv("MATRIX");
	if (var == NULL) {
		eprintf("MATRIX environment variable not set. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 3;
	}
	char* data = malloc((strlen(var) + 1) * sizeof(char));
	util_strlcpy(data, var, strlen(var) + 1);
	envdup = data;
	char* ip = data;
	char* portstr;
	if (strsep(&data, ":") == NULL) {
		eprintf("MATRIX environment variable doesn't contain a port seperator. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 3;
	}
	if ((portstr = strsep(&data, ",")) == NULL) { // can't find anything after : before ,
		eprintf("MATRIX environment variable doesn't contain port. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 3;
	}

	if (inet_aton(ip, &sio.sin_addr) == 0) {
		eprintf("MATRIX environment variable doesn't contain a valid IP. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 4;
	}

	char* xd;
	if ((xd = strsep(&data, "x")) == NULL) { // can't find anything after ,
		eprintf("MATRIX environment variable doesn't contain X size. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 3;
	}

	port = util_parse_int(portstr);
	if (port == 0) {
		eprintf("MATRIX environment variable doesn't contain a valid port. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 4;
	}
	sio.sin_port = htons(port);

	char* yd;
	if ((yd = strsep(&data, ",")) == NULL) { // can't find anything after ,
		eprintf("MATRIX environment variable doesn't contain Y size. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 3;
	}

	X_SIZE = util_parse_int(xd);
	if (X_SIZE == 0) {
		eprintf("MATRIX environment variable doesn't contain a X matrix size. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 4;
	}

	Y_SIZE = util_parse_int(yd);
	if (Y_SIZE == 0) {
		eprintf("MATRIX environment variable doesn't contain a Y matrix size. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 4;
	}

	// parse tiletype
	char* tilename = data;
	tiletype = -1;
	if (strcmp(tilename, "plain") == 0) tiletype = TILE_PLAIN;
	if (strcmp(tilename, "snake") == 0) tiletype = TILE_SNAKE;
	if (tiletype == -1) {
		eprintf("MATRIX environment variable doesn't contain a valid tiling type. Example: MATRIX=192.168.69.42:1234,16x8,snake\n");
		return 4;
	}

	// Allocate the message buffer.
  message = calloc((NUMPIX * 3) + 3, 1);
	assert(message); // 2lazy to handle it properly.
	message[0] = 0xAA;

	return 0;
}

int getx(void) {
	return X_SIZE;
}
int gety(void) {
	return Y_SIZE;
}

int ppos(int x, int y) {
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

int set(int x, int y, RGB *color) {
	int pos = (ppos(x, y) * 3) + 1;
	message[pos + 0] = color->red;
	message[pos + 1] = color->green;
	message[pos + 2] = color->blue;
	return 0;
}

int clear(void) {
	// message[1] to skip a byte (the 0xAA);
	memset(&message[1], '\0', NUMPIX);
	return 0;
};

int render(void) {
	// calculate checksum
	unsigned short chksum = 0;
	int i;
	for (i = 0; i <= (NUMPIX * 3); ++i)
		chksum += message[i];
	message[(NUMPIX * 3) + 1] = chksum >> 8; // high byte.
	message[(NUMPIX * 3) + 2] = chksum & 0x00FF; // low byte.

	// send udp packet.
	if (sendto(sock, message, ((NUMPIX * 3) + 3), 0, (struct sockaddr*) &sio, sizeof(sio)) == -1) {
		perror("out_udp: Failed to send UDP packet");
		return 5;
	}

	return 0;
}

ulong wait_until(ulong desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return wait_until_core(desired_usec);
}

int deinit(void) {
	close(sock);
	free(envdup);
	free(message);
	return 0;
}
