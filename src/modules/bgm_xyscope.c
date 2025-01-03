// An (XY) Oscilloscope, using ALSA audio sources as input.
// Anyone who makes the "this is out of scope" joke... Uh, vifino, are we allowed to say what happens to them?
// Apparently the answer is: "hahaha" "No. >:)" I assume by that they get nice happy hugs. -20kdc

#include <types.h>
#include <plugin.h>
#include <matrix.h>
#include <timers.h>
#include <graphics.h>
#include <oscore.h>
#include <stdio.h>
#include <random.h>
#include <alsa/asoundlib.h>
#include <math.h>

#define SAMPLE_RATE 48000
#define BUFFER_FRAMES 4096
#define TIMEOUT_FRAMES 480000
#define XAMUL 1
#define XADIV 2
// Phosphor gain control, controls how slow the beam must move to light a given line
#define PGAINMUL 3
#define PGAINDIV 2

#define PLOSSFAC ((BUFFER_FRAMES) / 64)

enum samplesize {
	SIZE_8, SIZE_16, SIZE_24, SIZE_32
};

static snd_pcm_t * scope_pcm;
// Details on the sample format before conversion.
// channel count - 2 enables XY mode, 1 acts like a primitive X oscilloscope
static int sf_chancount;
// xy mode, uses two channels
static int sf_usey;
static enum samplesize sf_sampsize;
static int sf_us;
static int sf_forceon;

static int camera_width;
static int camera_height;

// Read buffer
static byte * bufferA;

// Actual buffer
static byte bufferB[BUFFER_FRAMES * 2];

// Display buffer, split into:
// Section 0: Buffer 1 for CA (read)
// Section 1: Buffer 2 for CA (write)
// Section 2: Colourmapped data during final set stage
static byte * bufferC;

static int moduleno;

static int doshutdown;
static int dotimeout;

static oscore_task scope_task;

#define SM_ALGORITHM(sample, shr, sub) (((byte) (sample >> shr)) - sub)
#define LD_ALGORITHM(typ, shr, sub) \
	for (int indx = 0; indx < BUFFER_FRAMES; indx++) { \
		typ sampleA = ((typ *) bufferA)[indx * sf_chancount]; \
		bufferB[indx * 2] = SM_ALGORITHM(sampleA, shr, sub); \
		if (sf_chancount == 2 && sf_usey) { \
			typ sampleB = ((typ *) bufferA)[(indx * sf_chancount) + 1]; \
			bufferB[(indx * 2) + 1] = 255 - SM_ALGORITHM(sampleB, shr, sub); \
		} else { \
			bufferB[(indx * 2) + 1] = bufferB[indx * 2]; \
			bufferB[indx * 2] = ((xa++) * XAMUL) / XADIV; \
		} \
	}

static int pgain_func(int x, int y, void * pgf) {
	if (x < 0)
		return 0;
	if (y < 0)
		return 0;
	if (x >= camera_width)
		return 0;
	if (y >= camera_height)
		return 0;
	int pg = *((int*) pgf);
	// Phosphor gain
	int v = bufferC[x + (y * camera_width)];
	v += pg;
	if (v > 255)
		v = 255;
	bufferC[x + (y * camera_width)] = v;
	return 0;
}

static void * thread_func(void * ign) {
	snd_pcm_start(scope_pcm);
	int xa = 0;
	int lx = camera_width / 2;
	int ly = camera_height / 2;
	int pgm = camera_width;
	if (camera_height > pgm)
		pgm = camera_height;
	int timeout = 0;
	while (!doshutdown) {
		// -- Stream Audio --
		int frames = snd_pcm_readi(scope_pcm, bufferA, BUFFER_FRAMES);
		if (frames < 0)
			frames = snd_pcm_recover(scope_pcm, frames, 0);
		if (frames < 0)
			printf("Warning: reading totally failed: %i, %s\n", frames, snd_strerror(frames));
		if (sf_sampsize == SIZE_32) {
			if (sf_us) {
				LD_ALGORITHM(unsigned int, 24, 0);
			} else {
				LD_ALGORITHM(unsigned int, 24, 0x80);
			}
		} else if (sf_sampsize == SIZE_24) {
			if (sf_us) {
				LD_ALGORITHM(unsigned int, 16, 0);
			} else {
				LD_ALGORITHM(unsigned int, 16, 0x80);
			}
		} else if (sf_sampsize == SIZE_16) {
			if (sf_us) {
				LD_ALGORITHM(unsigned short, 8, 0);
			} else {
				LD_ALGORITHM(unsigned short, 8, 0x80);
			}
		} else if (sf_sampsize == SIZE_8) {
			if (sf_us) {
				LD_ALGORITHM(byte, 0, 0);
			} else {
				LD_ALGORITHM(byte, 0, 0x80);
			}
		}
		// This actually connects it all together
		for (int i = 0; i < frames; i++) {
			int x = (bufferB[i * 2] * (camera_width - 1)) / 255;
			int y = (bufferB[(i * 2) + 1] * (camera_height - 1)) / 255;
			// Phosphor gain
			int dx = x - lx;
			int dy = y - ly;
			// the abs is "magic" (-1 * -1 = 1)
			dx *= dx;
			dy *= dy;
			float dst = sqrtf(dx + dy) + 1;
			if (dy >= (camera_height / 8))
				timeout = TIMEOUT_FRAMES;
			int pgain = (int) (((PGAINMUL * pgm) / PGAINDIV) / dst);
			//pgain_func(x, y, &pgain);
			graphics_drawline_core(lx, ly, x, y, pgain_func, &pgain);
			lx = x;
			ly = y;
		}
		// -- Run CA --
		int camera_size = camera_width * camera_height;
		for (int i = 0; i < camera_size; i++) {
			byte srcM = bufferC[i];
			if (srcM >= PLOSSFAC) {
				srcM -= PLOSSFAC;
			} else {
				srcM = 0;
			}
			bufferC[camera_size + i] = srcM;
		}
		memcpy(bufferC, bufferC + camera_size, camera_size);
		// -- Add Timer --
		if (sf_forceon)
			timeout = TIMEOUT_FRAMES;
		if (timeout > 0) {
			timeout -= frames;
			dotimeout = timeout <= 0;
			timer_add(0, moduleno, 0, NULL);
			timers_wait_until_break();
		}
		oscore_task_yield();
	}
	snd_pcm_close(scope_pcm);
	free(bufferA);
	free(bufferC);
	return 0;
}

int init(int modulen, char* argstr) {
	moduleno = modulen;
	doshutdown = 0;
	camera_width = matrix_getx();
	camera_height = matrix_gety();
	bufferC = malloc(camera_width * camera_height * 3);
	if (!bufferC) {
		printf("Failed to allocate buffer C, dying\n");
		free(bufferC);
		return 1;
	}
	memset(bufferC, 0, camera_width * camera_height * 2);
	memset(bufferC + (camera_width * camera_height * 2), 255, camera_width * camera_height);
	sf_usey = 0;
	sf_forceon = 0;
	char * fx = getenv("SLED_SCOPE_USEY");
	if (fx)
		if (!strcmp(fx, "1"))
			sf_usey = 1;
	fx = getenv("SLED_SCOPE_FORCEON");
	if (fx)
		if (!strcmp(fx, "1"))
			sf_forceon = 1;
	// Can't rely on argstr right now, but allow it
	char * ourarg = argstr;
	if (!ourarg)
		ourarg = getenv("SLED_SCOPE_INPUT");
	if (!ourarg) {
		ourarg = "default";
		printf("You didn't specify an audio device for gfx_scope, so assuming you want 'default'.\n"
			"To resolve this, by device:\n");
		char ** devnames;
		if (snd_device_name_hint(-1, "pcm", (void ***) &devnames)) {
			printf("\n(ALSA wasn't available, so no device names for you.)\n\n");
		} else {
			char ** devnamep = devnames;
			while (*devnamep) {
				char * name = snd_device_name_get_hint(*devnamep, "NAME");
				if (name) {
					printf("For '%s', use:\n", *devnamep);
					printf("SLED_SCOPE_INPUT=\"%s\" ./sled\n", name);
					free(name);
				} else {
					printf("unknown dev: '%s'\n", *devnamep);
				}
				devnamep++;
			}
			snd_device_name_free_hint((void **) devnames);
			printf("\n");
		}
	}
	ourarg = strdup(ourarg);
	if (!ourarg) {
		printf("Couldn't strdup.\n");
		free(bufferC);
		return 1;
	}
	int code;
	if ((code = snd_pcm_open(&scope_pcm, ourarg, SND_PCM_STREAM_CAPTURE, 0))) {
		free(ourarg);
		printf("Couldn't open stream: %i\n", code);
		free(bufferC);
		return 1;
	}
	free(ourarg);

	sf_sampsize = SIZE_8;
	sf_us = 0;
	sf_chancount = sf_usey ? 2 : 1;

	if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_S8, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_8;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_16;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_S24, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_24;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_S32, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_32;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_8;
		sf_us = 1;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_U16, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_16;
		sf_us = 1;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_U24, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_24;
		sf_us = 1;
	} else if (!(code = snd_pcm_set_params(scope_pcm, SND_PCM_FORMAT_U32, SND_PCM_ACCESS_RW_INTERLEAVED, sf_chancount, SAMPLE_RATE, 1, 1000))) {
		sf_sampsize = SIZE_32;
		sf_us = 1;
	} else {
		printf("Couldn't convince ALSA to give sane settings: %i\n", code);
		snd_pcm_close(scope_pcm);
		free(bufferC);
		return 1;
	}

	int bytesPerSample;
	int bitsPerSample;
	switch (sf_sampsize) {
		case SIZE_8:
			bytesPerSample = 1;
			bitsPerSample = 8;
		break;
		case SIZE_16:
			bytesPerSample = 2;
			bitsPerSample = 16;
			break;
		case SIZE_24:
			bytesPerSample = 3;
			bitsPerSample = 24;
			break;
		case SIZE_32:
			bytesPerSample = 4;
			bitsPerSample = 32;
		break;
	}

	printf("Got B%c%dC%d\n", sf_us ? 'U' : 'S', bitsPerSample, sf_chancount);

	bufferA = malloc(BUFFER_FRAMES * sf_chancount * bytesPerSample);
	if (!bufferA) {
		printf("Couldn't allocate working buffer\n");
		snd_pcm_close(scope_pcm);
		free(bufferC);
		return 1;
	}

	scope_task = oscore_task_create("bgm_xyscope", thread_func, NULL);
	return 0;
}

// Colourmap
static RGB ddlc[] = {
	RGB(64, 255, 64),
	RGB(64, 240, 64),
	RGB(64, 224, 64),
	RGB(64, 208, 64),
	RGB(32, 192, 64),
	RGB(32, 176, 64),
	RGB(32, 160, 64),
	RGB(32, 128, 64),
	RGB(16, 64, 64),
	RGB(16, 48, 64),
	RGB(16, 32, 64),
	RGB(16, 16, 64),
	RGB(0, 0, 64),
	RGB(0, 0, 32),
	RGB(0, 0, 16),
	RGB(0, 0, 0)
};

static int get_cm(int ca) {
	return 15 - (ca >> 4);
}

void reset(int _modno) {
	int camera_size = camera_width * camera_height;
	memset(bufferC + (camera_size * 2), 255, camera_size);
}

int draw(int _modno, int argc, char* argv[]) {
	int camera_size = camera_width * camera_height;
	for (int i = 0; i < camera_size; i++) {
		int cm = get_cm(bufferC[i]);
		if (cm != bufferC[i + (camera_size * 2)]) {
			bufferC[i + (camera_size * 2)] = cm;
			matrix_set(i % camera_width, i / camera_width, ddlc[cm]);
		}
	}
	matrix_render();
	if (dotimeout)
		return 1;
	return 0;
}

void deinit(int _modno) {
	doshutdown = 1;
	oscore_task_join(scope_task);
}
