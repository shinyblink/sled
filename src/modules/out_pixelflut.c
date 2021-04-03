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
#define STRATEGY_LINEAR 0
#define STRATEGY_RANDOM 2

static int sock = -1;
struct sockaddr_in sio;
static int port;
static int X_SIZE;
static int Y_SIZE;
static int X_OFFSET;
static int Y_OFFSET;

#define NUMPIX (X_SIZE * Y_SIZE)
#define CHARS_PER_PIXEL 20

// Message will be one line of the following for each pixel
// "PX 0000 0000 FFFFFF\n"
static byte* buffer;
static char* message;
static uint32_t* shufflemap; 

static int strategy = STRATEGY_LINEAR;

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
	memset(&buffer[0], '\0', NUMPIX*3);
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
		eprintf("Pixelflut argstring not set. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}
	char* data = argstr;
	char* ip = data;
	char* portstr;
	if (strsep(&data, ":") == NULL) {
		eprintf("Pixelflut argstring doesn't contain a port seperator. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}
	if ((portstr = strsep(&data, ",")) == NULL) { // can't find anything after : before ,
		eprintf("Pixelflut argstring doesn't contain port. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}

	if (inet_aton(ip, &sio.sin_addr) == 0) {
		eprintf("Pixelflut argstring doesn't contain a valid IP. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 4;
	}

	char* xd;
	if ((xd = strsep(&data, "x")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain X size. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}

	port = util_parse_int(portstr);
	if (port == 0) {
		eprintf("Pixelflut argstring doesn't contain a valid port. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 4;
	}
	sio.sin_port = htons(port);

	char* yd;
	if ((yd = strsep(&data, "+")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain Y size. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}

	X_SIZE = util_parse_int(xd);
	if (X_SIZE == 0) {
		eprintf("Pixelflut argstring doesn't contain a X matrix size. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 4;
	}

	Y_SIZE = util_parse_int(yd);
	if (Y_SIZE == 0) {
		eprintf("Pixelflut argstring doesn't contain a Y matrix size. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 4;
	}

	
	if ((xd = strsep(&data, "+")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain X offset. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}
	if ((yd = strsep(&data, ",")) == NULL) { // can't find anything after ,
		eprintf("Pixelflut argstring doesn't contain Y offset. Example: -o pixelflut:192.168.69.42:1234,320x240+640+480\n");
		return 3;
	}
	X_OFFSET = util_parse_int(xd);
	if (X_OFFSET < 0) {
		X_OFFSET = 320;
	}

	Y_OFFSET = util_parse_int(yd);
	if (Y_OFFSET < 0) {
		Y_OFFSET = 240;
	}

	char* strategy_str = data;
	strategy = STRATEGY_LINEAR;
	if(data != 0) {
		if (strcmp(strategy_str, "random") == 0) strategy = STRATEGY_RANDOM;
		if (strcmp(strategy_str, "linear") == 0) strategy = STRATEGY_LINEAR;
	}
	// Allocate the message buffer.
	buffer = calloc((NUMPIX * 3), 1);
	message = calloc(NUMPIX * CHARS_PER_PIXEL, 1);
	assert(message); // 2lazy to handle it properly.
	assert(buffer);
	clear(0);

	// Free stuff.
	free(argstr);
	
	shufflemap = calloc(NUMPIX, 4);
	assert(shufflemap);
	for( int i = 0; i < NUMPIX; i++ ) {
		shufflemap[i] = i;
	}
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
}

int rx(int pos) {
	return (pos % X_SIZE);
}

int ry(int pos) {
	return (pos/X_SIZE);
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);
	
	int pos = (ppos(x, y) * 3);
	buffer[pos+0] = color.red;
	buffer[pos+1] = color.green;
	buffer[pos+2] = color.blue;
	
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < X_SIZE);
	assert(y < Y_SIZE);

	int pos = (ppos(x, y) * 3);
	return RGB(buffer[pos+0],buffer[pos+1],buffer[pos+2]);
}

int render(void) {
	int p, ap, bpos, ax, ay, mpos;
	for( int y = 0; y < Y_SIZE; y++) {
		for(int x = 0; x < X_SIZE; x++) {
			switch(strategy) {
				case STRATEGY_RANDOM :
					p = ppos(x,y);
					ap = shufflemap[p];
					bpos = ap * 3;
					ax = rx(ap);
					ay = ry(ap);
					mpos = p * CHARS_PER_PIXEL;
					//eprintf("x:%4d y:%4d -> p:%5d -> (bpos:%5d, mpos:%5d) -> ap:%5d -> (ax:%4d ay%4d)\n", x,y,p,bpos,mpos,ap,ax,ay);
					snprintf(&(message[mpos]), CHARS_PER_PIXEL, "PX %04d %04d %02x%02x%02x", ax+X_OFFSET, ay+Y_OFFSET, buffer[bpos+0], buffer[bpos+1], buffer[bpos+2]);
					message[mpos + CHARS_PER_PIXEL - 1] = '\n';
					break;
				case STRATEGY_LINEAR :
					p = ppos(x,y);
					bpos = p * 3;
					mpos = p * CHARS_PER_PIXEL;
					//eprintf("x:%4d y:%4d -> p:%5d -> (bpos:%5d, mpos:%5d) -> ap:%5d -> (ax:%4d ay%4d)\n", x,y,p,bpos,mpos,ap,ax,ay);
					snprintf(&(message[mpos]), CHARS_PER_PIXEL, "PX %04d %04d %02x%02x%02x", x+X_OFFSET, y+Y_OFFSET, buffer[bpos+0], buffer[bpos+1], buffer[bpos+2]);
					message[mpos + CHARS_PER_PIXEL - 1] = '\n';
					break;
			}
		}
	}

	send(sock, &message[0], NUMPIX * CHARS_PER_PIXEL, 0);
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
	free(buffer);
	free(shufflemap);
}
