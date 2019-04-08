// Module that shows the IP addresses of the system.
//
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
#include <matrix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <block_for.h>

#include "text.h"

#define BLOCK_TIME 3 // Execute at most every three minutes

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

int init (int moduleno, char *argstr) {
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

void reset(int _modno) {
	char buff[INET6_ADDRSTRLEN];
	char displaybuff[columncount];
	buff[0] = 0;

	reset_lines();

	struct ifaddrs *ifap;
	getifaddrs(&ifap);

	int displayed_interface_count = 0;

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
			displayed_interface_count++;

			snprintf(displaybuff, columncount, "%s", curr->ifa_name);
			lines[i] = text_render(displaybuff);
			i++;
			if (i >= linecount) break;

			snprintf(displaybuff, columncount, "	%s", buff);
			lines[i] = text_render(displaybuff);
			i++;
			if (i >= linecount) break;
		}
	}

	freeifaddrs(ifap);
	if (displayed_interface_count == 0){
		block_for(1);
		// block yourself for the next minute
		// if interfaces come up, it might get unblocked
	}
}

int draw(int _modno, int argc, char **argv) {
	if (check_block()) return 1;
	RGB black = RGB(0, 0, 0);
	for(int y = 0; y < matrix_gety(); y++) {
		for(int x = 0; x < matrix_getx(); x++) {
			matrix_set(x, y, black);
		}
	}

	for(int i = 0; i < linecount; i++) {
		if(lines[i] == NULL)
			continue;

		for (int y = 0; y < matrix_gety() && y < 8; y++) {
			for (int x = 0; x < matrix_getx(); x++) {
				byte v = text_point(lines[i], x, y);
				RGB color = RGB(v, v, v);
				matrix_set(x, i * 8 + y, color);
			}
		}
	}

	matrix_render();
	block_for(BLOCK_TIME);
	return 0;
}

void deinit(int _modno) {
	reset_lines();
	free(lines);
}
