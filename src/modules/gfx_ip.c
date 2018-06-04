// Module that shows the IP addresses of the system.

#include <types.h>
#include <matrix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "text.h"

static text **lines = NULL;
static int linecount = 0;
static int columncount = 0;
static const char *ignored_interfaces;

static void reset_lines(void)
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

	ignored_interfaces = getenv("SLED_IP_BLACKLIST");
	if(ignored_interfaces == NULL)
		ignored_interfaces = "lo";

	return 0;
}

void reset(void) {
	char buff[INET6_ADDRSTRLEN];
	char displaybuff[columncount];
	buff[0] = 0;

	reset_lines();

	struct ifaddrs *ifap;
	getifaddrs(&ifap);

	int i = 0;
	for(struct ifaddrs *curr = ifap; curr != NULL; curr = curr->ifa_next) {
		const char *ignored = strstr(ignored_interfaces, curr->ifa_name);
		if(ignored != NULL) {
			size_t len = strlen(curr->ifa_name);
			if(ignored[len] == 0 || ignored[len] == ',')
				continue;
		}

		struct sockaddr_in *addr = (struct sockaddr_in *)curr->ifa_addr;
		if(inet_ntop(curr->ifa_addr->sa_family, &(addr->sin_addr), buff, INET6_ADDRSTRLEN) != NULL) {
			snprintf(displaybuff, columncount, "%s", curr->ifa_name);
			lines[i] = text_render(displaybuff);
			i++;

			snprintf(displaybuff, columncount, "    %s", buff);
			lines[i] = text_render(displaybuff);
			i++;
		}
	}

	freeifaddrs(ifap);
}

int draw(int argc, char **argv) {
	RGB black = RGB(0, 0, 0);
	for(int y = 0; y < matrix_gety(); y++) {
		for(int x = 0; x < matrix_getx(); x++) {
			matrix_set(x, y, &black);
		}
	}

	for(int i = 0; i < linecount; i++) {
		if(lines[i] == NULL)
			continue;

		for (int y = 0; y < matrix_gety() && y < 8; y++) {
			for (int x = 0; x < matrix_getx(); x++) {
				int v = text_point(lines[i], x, y) ? 255 : 0;
				RGB colour = RGB(v, v, v);
				matrix_set(x, i * 8 + y, &colour);
			}
		}
	}

	matrix_render();
	return 0;
}

int deinit(void) {
	reset_lines();
	return 0;
}
