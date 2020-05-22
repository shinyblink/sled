// Pixelflut output.
//
//
// Copyright (c) 2020, Dave "anathem" Kliczbor <maligree@gmx.de>
//
// Built as a modification of out_udp.c, which is:
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

#define NUMPIX (X_SIZE * Y_SIZE)
#define CHARS_PER_PIXEL 20

// Message will be:
// 0xAA <R,G,B bytes..> <2 bytes checksum, unsigned short, hi, low>
static char* message;
static uint32_t* shufflemap; 

void shuffle(uint32_t *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          uint32_t t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

int clear(int _modno) {
	memset(&message[0], '\n', NUMPIX*CHARS_PER_PIXEL);
	return 0;
};

int init (int moduleno, char* argstr) {
	// Partially initialize the socket.
	if ((sock=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("out_pixelflut: Failed to initialize socket");
		return 2;
	}
	memset((char *) &sio, 0, sizeof(struct sockaddr_in));
	sio.sin_family = AF_INET;

	// Parse string. This sucks.
	if (argstr == NULL) {
		eprintf("Pixelflut argstring not set. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 3;
	}
	char* data = argstr;
	char* ip = data;
	char* portstr;
	if (strsep(&data, ":") == NULL) {
		eprintf("Pixelflut argstring doesn't contain a port seperator. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 3;
	}
	if ((portstr = strsep(&data, ",")) == NULL) { // can't find anything after : before ,
		eprintf("Pixelflut argstring doesn't contain port. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 3;
	}

	if (inet_aton(ip, &sio.sin_addr) == 0) {
		eprintf("Pixelflut argstring doesn't contain a valid IP. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 4;
	}

	char* xd;
	if ((xd = strsep(&data, "x")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain X size. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 3;
	}

	port = util_parse_int(portstr);
	if (port == 0) {
		eprintf("Pixelflut argstring doesn't contain a valid port. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 4;
	}
	sio.sin_port = htons(port);

	char* yd;
	if ((yd = strsep(&data, ",")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain Y size. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 3;
	}

	X_SIZE = util_parse_int(xd);
	if (X_SIZE == 0) {
		eprintf("Pixelflut argstring doesn't contain a X matrix size. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 4;
	}

	Y_SIZE = util_parse_int(yd);
	if (Y_SIZE == 0) {
		eprintf("Pixelflut argstring doesn't contain a Y matrix size. Example: -o pixelflut:192.168.69.42:1234,1024x768\n");
		return 4;
	}

	// Allocate the message buffer.
	message = calloc((NUMPIX * CHARS_PER_PIXEL) + 1, 1);
	assert(message); // 2lazy to handle it properly.
	clear(0);

	// Free stuff.
	free(argstr);
	
	shufflemap = calloc(NUMPIX, 4);
	shuffle(shufflemap, NUMPIX);
	
	if( connect(sock, (struct sockaddr*)&sio, sizeof(sio)) != 0 ) {
		eprintf("Cannot connect to pixelflut\n");
		return 5;
	}

	return 0;
}

int getx(int _modno) {
	return X_SIZE;
}
int gety(int _modno) {
	return Y_SIZE;
}

int ppos(int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);
	return (x + (y*X_SIZE));
//	int ret = shufflemap[(x + (y * X_SIZE))];
//	assert(ret < NUMPIX);
//	return ret;
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);
	
	int pos = (ppos(x, y) * CHARS_PER_PIXEL);
	sprintf(&(message[pos]), "PX %04d %04d %02x%02x%02x\n", x, y, color.red, color.green, color.blue);
	
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);

	return RGB(0,0,0);
}

int render(void) {
	// send udp packet.
	//eprintf("%s---\n", message);
	send(sock, &message[0], (NUMPIX*CHARS_PER_PIXEL)+1, 0);
	clear(0);
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
