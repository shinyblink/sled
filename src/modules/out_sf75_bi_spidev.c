// First pass at something to send the matrix to sled-fpga-hub75,
//  mostly according to Vifino's design (pixel layout changed)
// "Here goes nothing", basically. - 20kdc

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <timers.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

// All necesssary data regarding a connected IOP (presumably a TinyFPGA BX)
typedef struct {
	const char * filename;
	int x, y, w, h;
	// ---
	int fd;
	byte * buf;
} matrixdriver_t;

#define WORLD_X 128
#define WORLD_Y 128
static matrixdriver_t matrices[] = {
	{"/dev/spidev0.0", 0, 0, 128, 64},
	{"/dev/spidev0.1", 0, 128, 128, 64},
	{NULL}
};

int init(void) {
	int i = 0;
	while (matrices[i].filename) {
		matrices[i].fd = open(matrices[i].filename, O_RDWR);
		matrices[i].buf = malloc(matrices[i].w * matrices[i].h * 2);
		if (!matrices[i].buf) {
			printf("sf75_bi_spidev ran out of memory allocating buffer\n");
			for (int p = 0; p < i; p++) {
				free(matrices[i].buf);
				close(matrices[i].fd);
			}
			return 1;
		}
		if (matrices[i].fd < 0) {
			printf("sf75_bi_spidev couldn't open: %s\n", matrices[i].filename);
			for (int p = 0; p < i; p++) {
				free(matrices[i].buf);
				close(matrices[i].fd);
			}
			return 1;
		}
		char mode = 0;
		unsigned maxspeed = 16000000;
		// All of these have correct parameters given 0.
		ioctl(matrices[i].fd, SPI_IOC_WR_MODE, &mode);
		ioctl(matrices[i].fd, SPI_IOC_WR_LSB_FIRST, &mode);
		ioctl(matrices[i].fd, SPI_IOC_WR_BITS_PER_WORD, &mode);
		ioctl(matrices[i].fd, SPI_IOC_WR_MAX_SPEED_HZ, &maxspeed);
		i++;
	}
	return 0;
}

int getx(int _modno) {
	return WORLD_X;
}
int gety(int _modno) {
	return WORLD_Y;
}

int set(int _modno, int x, int y, RGB color) {
	if (x < 0 || y < 0)
		return 1;
	if (x >= WORLD_X || y >= WORLD_Y)
		return 2;

	int i = 0;
	while (matrices[i].filename) {
		if ((x < matrices[i].x || x >= (matrices[i].x + matrices[i].w)) ||
			(y < matrices[i].y || y >= (matrices[i].y + matrices[i].h))) {
			i++;
			continue;
		}
		x -= matrices[i].x;
		y -= matrices[i].y;
		uint rgbt = 0;
		rgbt |= (color.red & 0x1F) << 10;
		rgbt |= (color.green & 0x1F) << 5;
		rgbt |= color.blue & 0x1F;
		byte high, low;
		high = rgbt >> 8;
		low = rgbt;
		matrices[i].buf[((x + (y * matrices[i].w)) * 2) + 0] = high;
		matrices[i].buf[((x + (y * matrices[i].w)) * 2) + 1] = low;
		return 0;
	}
	// Setting pixels? Nah, we're good.
	return 0;
}

int clear(int _modno) {
	int i = 0;
	while (matrices[i].filename) {
		memset(matrices[i].buf, 0, matrices[i].w * matrices[i].h);
		i++;
	}
	return 0;
};

int render(void) {
	int i = 0;
	// While this backend really doesn't feel like it, it does it anyway
	while (matrices[i].filename) {
		write(matrices[i].fd, matrices[i].buf, matrices[i].w * matrices[i].h * 2);
		i++;
	}
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	// Hey, we can just delegate work to someone else. Yay!
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	int i = 0;
	while (matrices[i].filename) {
		free(matrices[i].buf);
		close(matrices[i].fd);
		i++;
	}
}
