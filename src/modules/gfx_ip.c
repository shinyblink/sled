#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <string.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "text.h"

#define FRAMETIME (T_SECOND)
#define FRAMES (RANDOM_TIME)
#define CHARS_FULL 8 // 20:15:09, must also hold 20:15 (small)

static text* rendered = NULL;

int init(int modno, char *argstr) {
	// max digit size is 4, plus 3 for ., (4 * 3) * 4 + 3 * 3 = 57
	//TODO split ip into multiple rows
	if (matrix_getx() < 57)
		return 1; // not enough X to be usable
	if (matrix_gety() < 7)
		return 1; // not enough Y to be usable

	//TODO allow to customize these, can we use argstr here?
	int target_family = AF_INET;
	const char *target_interface = "eth0";

	char buff[INET_ADDRSTRLEN];
	buff[0] = 0;

	struct ifaddrs *ifap;
	getifaddrs(&ifap);

	for(struct ifaddrs *curr = ifap; curr != NULL; curr = curr->ifa_next) {
		struct sockaddr *addr = curr->ifa_addr;
		if(addr->sa_family == target_family && strcmp(curr->ifa_name, target_interface) == 0) {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)curr->ifa_addr;
			inet_ntop(target_family, &(addr_in->sin_addr), buff, INET_ADDRSTRLEN);
			break;
		}
	}

	if(buff[0] == 0)
		return 1;

	rendered = text_render(buff);
	if(rendered == NULL)
		return 1;

	return 0;
}

void reset() {
}

int draw(int argc, char **argv) {
	for (int y = 0; y < matrix_gety(); y++) {
		for (int x = 0; x < matrix_getx(); x++) {
			int v = text_point(rendered, x, y) ? 255 : 0;
			RGB color = RGB(v, v, v);
			matrix_set(x, y, &color);
		}
	}

	matrix_render();
	return 0;
}

int deinit() {
	// This acts conditionally on rendered being non-NULL.
	text_free(rendered);
	return 0;
}
