// Linux Framebuffer (fbdev)
// Only accepts specific framebuffer types for code simplicity.
// In theory this helps w/ BSD support.

#include <types.h>
#include <timers.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stropts.h>
#include <stdio.h>
#include <stdlib.h>

static char * buf;

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
#include <linux/fb.h>
static int query_device(void) {
	// Framebuffer access
	char * env = getenv("FRAMEBUFFER");
	if (!env)
		env = "/dev/fb0";
	fbdev_fd = open(env, O_RDWR);
	if (fbdev_fd < 0)
		return 1;
	// 20kdc's config (PITCAIRN over HDMI)
	// fbdev_w = 1920;
	// fbdev_h = 1080;
	// fbdev_flags = SLEDFB_P4EN | SLEDFB_BGR; // 0x9
	struct fb_var_screeninfo ifo;
	struct fb_fix_screeninfo xifo;
	if (ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &ifo) == -1) {
		close(fbdev_fd);
		return 1;
	}
	if (ioctl(fbdev_fd, FBIOGET_FSCREENINFO, &xifo) == -1) {
		close(fbdev_fd);
		return 1;
	}
	fbdev_w = ifo.xres;
	fbdev_h = ifo.yres;
	fbdev_flags = 0;
	// Guess details
	if (xifo.type != FB_TYPE_PACKED_PIXELS) {
		if (xifo.type != FB_TYPE_PLANES) {
			fprintf(stderr, "FB: Expected PACKED_PIXELS (0) or PLANES (1)-type display, got %i\n", xifo.type);
			return 1;
		}
		fbdev_flags |= SLEDFB_PLANAR;
	}
	if (ifo.bits_per_pixel != 24) {
		if (ifo.bits_per_pixel != 32) {
			fprintf(stderr, "FB: Expected 24/32-bit display, got %i\n", ifo.bits_per_pixel);
			return 1;
		}
		fbdev_flags |= SLEDFB_P4EN;
		if (ifo.transp.length) {
			fbdev_flags |= SLEDFB_P4AI;
			if (ifo.transp.offset < ifo.red.offset)
				fbdev_flags |= SLEDFB_P4AF;
		}
	}
	if (ifo.red.offset > ifo.blue.offset)
		fbdev_flags |= SLEDFB_BGR;
	// Final debug print
	fprintf(stderr, "FB: \"%s\" -> %i x %i, flags %02x\n", xifo.id, fbdev_w, fbdev_h, fbdev_flags);
	return 0;
}

int init(void) {
	if (query_device())
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

int getx(void) {
	return fbdev_w;
}
int gety(void) {
	return fbdev_h;
}

int set(int x, int y, RGB *color) {
	if (x < 0)
		return 1;
	if (y < 0)
		return 1;
	if (x >= fbdev_w)
		return 1;
	if (y >= fbdev_h)
		return 1;
	if (!(fbdev_flags & SLEDFB_PLANAR)) {
		int i = (x + (y * fbdev_w)) * ((fbdev_flags & SLEDFB_P4EN) ? 4 : 3);
		if (fbdev_flags & SLEDFB_P4AF)
			i++;
		buf[i] = (fbdev_flags & SLEDFB_BGR) ? color->blue : color->red;
		buf[i + 1] = color->green;
		buf[i + 2] = (fbdev_flags & SLEDFB_BGR) ? color->red : color->blue;
	} else {
		int i = x + (y * fbdev_w);
		int p = fbdev_w * fbdev_h;
		if (fbdev_flags & SLEDFB_P4AF)
			i += p;
		buf[i] = (fbdev_flags & SLEDFB_BGR) ? color->blue : color->red;
		buf[i + p] = color->green;
		buf[i + (p * 2)] = (fbdev_flags & SLEDFB_BGR) ? color->red : color->blue;		
	}
	return 0;
}

int clear(void) {
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
			char * buf2 = buf;
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

ulong wait_until(ulong desired_usec) {
	return wait_until_core(desired_usec);
}

int deinit(void) {
	close(fbdev_fd);
	return 0;
}
