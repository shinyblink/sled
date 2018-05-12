#include <types.h>
#include <matrix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "text.h"

#define FRAMETIME (T_SECOND)
#define FRAMES (10)
#define CHARS_FULL 8 // 20:15:09, must also hold 20:15 (small)

static text **lines = NULL;
static int linecount = 0;
static int columncount = 0;

void reset_lines()
{
	for(int i = 0; i < linecount; i++) {
		text_free(lines[i]);
		lines[i] = NULL;
	}
}

int init(int modno, char *argstr) {
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable

	linecount = matrix_gety() / 8;
	columncount = matrix_getx() / 3;

	lines = calloc(linecount, sizeof(text *));

	return 0;
}

void reset() {
    char buff[INET6_ADDRSTRLEN];
	char displaybuff[columncount];
	buff[0] = 0;

	reset_lines();

	struct ifaddrs *ifap;
	getifaddrs(&ifap);

	int i = 0;
	for(struct ifaddrs *curr = ifap; curr != NULL; curr = curr->ifa_next) {
		struct sockaddr_in *addr = (struct sockaddr_in *)curr->ifa_addr;

		if(inet_ntop(curr->ifa_addr->sa_family, &(addr->sin_addr), buff, INET6_ADDRSTRLEN) != NULL) {
			snprintf(displaybuff, columncount, "%s", curr->ifa_name, buff);
			lines[i] = text_render(displaybuff);
			i++;

			snprintf(displaybuff, columncount, "    %s", buff, buff);
			lines[i] = text_render(displaybuff);
			i++;
		}
	}

	freeifaddrs(ifap);
}

int draw(int argc, char **argv) {
	for(int i = 0; i < linecount; i++) {
		if(lines[i] == NULL)
			continue;

		for (int y = 0; y < matrix_gety() && y < 8; y++) {
			for (int x = 0; x < matrix_getx(); x++) {
				int v = text_point(lines[i], x, y) ? 255 : 0;
				RGB color = RGB(v, v, v);
				matrix_set(x, i * 8 + y, &color);
			}
		}
	}

	matrix_render();
	return 0;
}

int deinit() {
	reset_lines();
	return 0;
}
