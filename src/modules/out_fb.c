// Linux Framebuffer (fbdev)
// Only accepts specific framebuffer types for code simplicity.
// In theory this helps w/ BSD support.

#include <types.h>
#include <timers.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static byte* buf;

// 32bpp
#define SLEDFB_P4EN 1
// Alpha is *1st* byte
#define SLEDFB_P4AF 2
// Alpha needs to be set to 255
#define SLEDFB_P4AI 4
// BGR, not RGB
#define SLEDFB_BGR 8
// Planar
#define SLEDFB_PLANAR 16

static int fbdev_w, fbdev_h, fbdev_fd;
static int fbdev_flags;

// Hardware Detection Routine - platform specific
#if defined(__linux__)
#include <linux/fb.h>
#endif

#ifdef __FreeBSD__
#include <machine/param.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#include <sys/kbio.h>
#include <sys/types.h>
#endif

#if !defined(__linux__) && !defined(__FreeBSD__)
#error out_fb does not support this OS. Linux and FreeBSD only.
#endif

static int query_device(char* device) {
	// Framebuffer access
	fbdev_fd = open(device, O_RDWR);
	if (fbdev_fd < 0) {
		eprintf("FB: Failed to open framebuffer %s.\n", device);
		return 1;
	}
	// 20kdc's config (PITCAIRN over HDMI)
	// fbdev_w = 1920;
	// fbdev_h = 1080;
	// fbdev_flags = SLEDFB_P4EN | SLEDFB_BGR; // 0x9
#ifdef __linux__
	struct fb_var_screeninfo ifo;
	struct fb_fix_screeninfo xifo;
	if (ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &ifo) == -1) {
		eprintf("FB: Couldn't get var screen info.\n");
		close(fbdev_fd);
		return 1;
	}
	if (ioctl(fbdev_fd, FBIOGET_FSCREENINFO, &xifo) == -1) {
		eprintf("FB: Couldn't get fix screen info.\n");
		close(fbdev_fd);
		return 1;
	}
	fbdev_w = ifo.xres;
	fbdev_h = ifo.yres;
#elif defined(__FreeBSD__)
	/*
	video_adapter_info_t ainfo;
	video_info_t vinfo;
	if (ioctl(fbdev_fd, FBIO_ADPINFO, &ainfo)) {
		eprintf("FB: Couldn't get adapter info.\n");
		close(fbdev_fd);
		return 1;
	}
	if (ioctl(fbdev_fd, FBIO_GETMODE, &vinfo) < 0) {
		eprintf("FB: Couldn't get mode.\n");
		close(fbdev_fd);
		return 1;
	}
	fbdev_w = vinfo.vi_width;
	fbdev_h = vinfo.vi_height;
	*/
	// since the above is broken, hardcoding, yay.
	fbdev_w = 1366;
	fbdev_h = 768;
	fbdev_flags = SLEDFB_P4EN;
#endif
	fbdev_flags = 0;

#ifdef __linux__
	// Guess details
	if (xifo.type != FB_TYPE_PACKED_PIXELS) {
		if (xifo.type != FB_TYPE_PLANES) {
			fprintf(stderr, "FB: Expected PACKED_PIXELS (0) or PLANES (1)-type display, got %i\n", xifo.type);
			return 1;
		}
		fbdev_flags |= SLEDFB_PLANAR;
	}
	int bpp = ifo.bits_per_pixel;
#endif
#ifdef __FreeBSD__
int bpp = 24; //vinfo.bpp;
#endif
	if (bpp != 24) {
		if (bpp != 32) {
			fprintf(stderr, "FB: Expected 24/32-bit display, got %i\n", bpp);
			return 1;
		}
		fbdev_flags |= SLEDFB_P4EN;
#ifdef __linux__
		if (ifo.transp.length) {
			fbdev_flags |= SLEDFB_P4AI;
			if (ifo.transp.offset < ifo.red.offset)
				fbdev_flags |= SLEDFB_P4AF;
		}
#endif
}

#ifdef __linux__
	if (ifo.red.offset > ifo.blue.offset)
		fbdev_flags |= SLEDFB_BGR;
#endif
	// Final debug print
	fprintf(stderr, "FB: \"%s\" -> %i x %i, flags %02x\n", device, fbdev_w, fbdev_h, fbdev_flags);
	return 0;
}

int init (int moduleno, char* argstr) {
	char* device;
	if (argstr) {
		device = strdup(argstr);
		free(argstr);
	} else {
		device = getenv("FRAMEBUFFER");
		if (!device)
			device = "/dev/fb0";
	}

	if (query_device(device))
		return 2;
	if (fbdev_flags & SLEDFB_P4EN) {
		buf = malloc(fbdev_w * fbdev_h * 4);
	} else {
		buf = malloc(fbdev_w * fbdev_h * 3);
	}
	if (!buf) {
		close(fbdev_fd);
		return 2;
	}
	return 0;
}

// Everything under here should be kernel-independent.
// Note the large amount of options and interactions involved -
//  the idea is that while making this support every fbdev format is a kerneldev's pipe dream,
//  the program should at least be able to support some fairly common but varied configurations.
//  - 20kdc

int getx(int _modno) {
	return fbdev_w;
}
int gety(int _modno) {
	return fbdev_h;
}

int set(int _modno, int x, int y, RGB color) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < fbdev_w);
	assert(y < fbdev_h);

	if (!(fbdev_flags & SLEDFB_PLANAR)) {
		int i = (x + (y * fbdev_w)) * ((fbdev_flags & SLEDFB_P4EN) ? 4 : 3);
		if (fbdev_flags & SLEDFB_P4AF)
			i++;
		buf[i] = (fbdev_flags & SLEDFB_BGR) ? color.blue : color.red;
		buf[i + 1] = color.green;
		buf[i + 2] = (fbdev_flags & SLEDFB_BGR) ? color.red : color.blue;
	} else {
		int i = x + (y * fbdev_w);
		int p = fbdev_w * fbdev_h;
		if (fbdev_flags & SLEDFB_P4AF)
			i += p;
		buf[i] = (fbdev_flags & SLEDFB_BGR) ? color.blue : color.red;
		buf[i + p] = color.green;
		buf[i + (p * 2)] = (fbdev_flags & SLEDFB_BGR) ? color.red : color.blue;
	}
	return 0;
}

RGB get(int _modno, int x, int y) {
	assert(x >= 0);
	assert(y >= 0);
	assert(x < fbdev_w);
	assert(y < fbdev_h);

	if (!(fbdev_flags & SLEDFB_PLANAR)) {
		int i = (x + (y * fbdev_w)) * ((fbdev_flags & SLEDFB_P4EN) ? 4 : 3);
		if (fbdev_flags & SLEDFB_P4AF)
			i++;
		return RGB((fbdev_flags & SLEDFB_BGR) ? buf[i + 2] : buf[i], buf[i + 1], (fbdev_flags & SLEDFB_BGR) ? buf[i] : buf[i + 2]); 
	} else {
		int i = x + (y * fbdev_w);
		int p = fbdev_w * fbdev_h;
		if (fbdev_flags & SLEDFB_P4AF)
			i += p;
		return RGB((fbdev_flags & SLEDFB_BGR) ? buf[i + (p * 2)] : buf[i], buf[i + p], (fbdev_flags & SLEDFB_BGR) ? buf[i] : buf[i + (p * 2)]); 
	}
}


int clear(int _modno) {
	int i;
	if (fbdev_flags & SLEDFB_P4AI) {
		if (!(fbdev_flags & SLEDFB_PLANAR)) {
			for (i = 0; i < fbdev_w * fbdev_h * 4; i += 4) {
				buf[i] = (fbdev_flags & SLEDFB_P4AF) ? 255 : 0;
				buf[i + 1] = 0;
				buf[i + 2] = 0;
				buf[i + 3] = (fbdev_flags & SLEDFB_P4AF) ? 0 : 255;
			}
		} else {
			memset(buf, 0, fbdev_w * fbdev_h * 4);
			unsigned char * buf2 = buf;
			if (!(fbdev_flags & SLEDFB_P4AF))
				buf2 += (fbdev_w * fbdev_h) * 3;
			memset(buf2, 255, fbdev_w * fbdev_h);
		}
	} else {
		memset(buf, 0, fbdev_w * fbdev_h * ((fbdev_flags & SLEDFB_P4EN) ? 4 : 3));
	}
	return 0;
};

int render(void) {
	lseek(fbdev_fd, 0, SEEK_SET);
	write(fbdev_fd, buf, fbdev_w * fbdev_h * ((fbdev_flags & SLEDFB_P4EN) ? 4 : 3));
	return 0;
}

oscore_time wait_until(int _modno, oscore_time desired_usec) {
	return timers_wait_until_core(desired_usec);
}

void wait_until_break(int _modno) {
	timers_wait_until_break_core();
}

void deinit(int _modno) {
	close(fbdev_fd);
}
