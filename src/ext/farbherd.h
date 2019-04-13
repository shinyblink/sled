// farbherd - animation format & tools, designed to be similar to farbfeld.
// Written in 2018 by 20kdc <asdd2808@gmail.com>, vifino <vifino@tty.sh> and contributors.
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
// This software is distributed without any warranty.
// You should have received a copy of the CC0 Public Domain Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

// I know, I know, not standardized.
// But painless fallback.
#if defined(__linux__)
#include <endian.h>
// These are defined as __uint32_identity and the like on BIG_ENDIAN systems.
// These in turn are defined as inline functions, so it's harmless.
#define farbherd_be32toh be32toh
#define farbherd_htobe32 htobe32
#define farbherd_be16toh be16toh
#define farbherd_htobe16 htobe16
#else
#include <arpa/inet.h>
#define farbherd_be32toh ntohl
#define farbherd_htobe32 htonl
#define farbherd_be16toh ntohs
#define farbherd_htobe16 htons
#endif

#define DEBUGF(...) fprintf(stderr, __VA_ARGS__)

// Common stuff for the farbherd tools
#define FARBHERD_FLAG_LOOP 1

// Farbfeld, spec by suckless
typedef struct {
	uint32_t width;
	uint32_t height;
} farbfeld_header_t;

// Farbherd, spec by shinyblink
typedef struct {
	farbfeld_header_t imageHead;

	// Flags.
	uint32_t flags;
	// Frames, might be zero for livestreams or non-counted.
	uint32_t frameCount;

	// Extension size, metadata, etc..
	uint32_t fileExtSize, frameExtSize;
	// In seconds
	uint32_t frameTimeDiv, frameTimeMul;
	// fileExt is between the header and the frames. frameExt is before each frame.
	// This is malloced during read, you have to free it yourself.
	uint8_t * fileExtData;
} farbherd_header_t;

// Readers:
// These return non-zero on error. Note that on error, the target is at an undefined position.
// Also note that on error, any buffers that would be malloced during the call are either freed or aren't malloced.
// Meanwhile, on non-error, all buffers are safe to free() - they are malloc'd or are NULL, defined to be a NOP for free.
inline static int farbherd_read_buffer(FILE * target, void * buf, size_t amount) {
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise(fileno(target), 0, amount, POSIX_FADV_SEQUENTIAL);
#endif
	do {
		size_t a = fread(buf, 1, amount, target);
		if (!a)
			return 1;
		amount -= a;
		buf = (char*) buf + a;
	} while (amount != 0);
	return 0;
}

inline static int farbherd_read_int(FILE * target, uint32_t * i) {
	uint32_t tmp;
	if (farbherd_read_buffer(target, &tmp, sizeof(uint32_t)))
		return 1;
	*i = farbherd_be32toh(tmp);
	return 0;
}

inline static int farbherd_read_farbfeld_header(FILE * target, farbfeld_header_t * fht) {
	char buf[9];
	if (farbherd_read_buffer(target, buf, 8))
		return 1;
	buf[8] = 0;
	if (strncmp(buf, "farbfeld", 8)) {
		DEBUGF("read %s, not farbfeld", buf);
		return 1;
	}
	if (farbherd_read_int(target, &(fht->width)))
		return 1;
	if (farbherd_read_int(target, &(fht->height)))
		return 1;
	return 0;
}

inline static int farbherd_read_farbherd_header(FILE * target, farbherd_header_t * fht) {
	char buf[8];
	if (farbherd_read_buffer(target, buf, 8))
		return 1;
	if (strncmp(buf, "farbherd", 8) != 0)
		return 1;

	if (farbherd_read_int(target, &(fht->imageHead.width)))
		return 1;
	if (farbherd_read_int(target, &(fht->imageHead.height)))
		return 1;
	if (!(fht->imageHead.width && fht->imageHead.height))
		return 1;

	if (farbherd_read_int(target, &(fht->flags)))
		return 1;
	if (farbherd_read_int(target, &(fht->frameCount)))
		return 1;

	if (farbherd_read_int(target, &(fht->fileExtSize)))
		return 1;
	if (farbherd_read_int(target, &(fht->frameExtSize)))
		return 1;

	if (farbherd_read_int(target, &(fht->frameTimeDiv)))
		return 1;
	if (farbherd_read_int(target, &(fht->frameTimeMul)))
		return 1;
	if (!fht->frameTimeDiv)
		return 1;
	if (!fht->frameTimeMul)
		return 1;

	if (fht->fileExtSize) {
		fht->fileExtData = malloc(fht->fileExtSize);
		if (!fht->fileExtData)
			return 1;
		if (farbherd_read_buffer(target, fht->fileExtData, fht->fileExtSize)) {
			free(fht->fileExtData);
			return 1;
		}
	}
	return 0;
}

// Output functions.
inline static int farbherd_write_int(FILE * target, uint32_t v) {
	uint32_t be = farbherd_htobe32(v);
	if(!fwrite(&be, sizeof(uint32_t), 1, target))
		return 1;
	return 0;
}

inline static void farbherd_write_farbfeld_header(FILE * target, farbfeld_header_t filehdr) {
	fputs("farbfeld", target);
	farbherd_write_int(target, filehdr.width);
	farbherd_write_int(target, filehdr.height);
}

inline static int farbherd_write_farbherd_header(FILE * target, farbherd_header_t filehdr) {
	fputs("farbherd", target);
	if (farbherd_write_int(target, filehdr.imageHead.width))
		return 1;
	if (farbherd_write_int(target, filehdr.imageHead.height))
		return 1;
	if(farbherd_write_int(target, filehdr.flags))
		return 1;
	if (farbherd_write_int(target, filehdr.frameCount))
		return 1;

	if (farbherd_write_int(target, filehdr.fileExtSize))
		return 1;
	if (farbherd_write_int(target, filehdr.frameExtSize))
		return 1;

	if(farbherd_write_int(target, filehdr.frameTimeDiv))
		return 1;
	if(farbherd_write_int(target, filehdr.frameTimeMul))
		return 1;
	if (filehdr.fileExtSize)
		if (fwrite(filehdr.fileExtData, filehdr.fileExtSize, 1, target) != 1)
			return 1;

	return 0;
}

// Delta/Pixel endianness
inline static void farbherd_endian_datain(uint16_t * netdata, size_t datasize) {
	for (size_t n = 0; n < datasize; n += sizeof(uint16_t)) {
		*netdata = farbherd_be16toh(*netdata);
		netdata++;
	}
}
inline static void farbherd_endian_dataout(uint16_t * hostdata, size_t datasize) {
	for (size_t n = 0; n < datasize; n += sizeof(uint16_t)) {
		*hostdata = farbherd_htobe16(*hostdata);
		hostdata++;
	}
}

// Frame handling utilities

typedef struct {
	uint8_t * frameExtData;
	// NOTE! The byte order of these is not changed during frame read/write.
	// The delta operation functions use htons and ntohs.
	uint16_t * deltas;
} farbherd_frame_t;

// NOTE: A 'datasize' is the farbfeld data size/farbherd delta size.
//       A 'framesize' includes farbherd frame extended data.
//       Both of these are in malloc-friendly units.

inline static size_t farbherd_datasize(farbfeld_header_t imageHead) {
	return ((size_t) imageHead.width) * ((size_t) imageHead.height) * ((size_t) 8);
}

// NOTE: Since there's a lot of frames in a long farbherd file, this has an additional 'allocate' step.

inline static int farbherd_init_farbherd_frame(farbherd_frame_t * ft, farbherd_header_t mhead) {
	ft->frameExtData = 0;
	ft->deltas = 0;
	if (mhead.frameExtSize) {
		ft->frameExtData = malloc(mhead.frameExtSize);
		if (!ft->frameExtData)
			return 1;
	}
	size_t datasize = farbherd_datasize(mhead.imageHead);
	if (datasize) {
		ft->deltas = malloc(datasize);
		if (!ft->deltas) {
			// NOP for NULL
			free(ft->frameExtData);
			return 1;
		}
	}
	return 0;
}

inline static int farbherd_read_farbherd_frame(FILE * input, farbherd_frame_t * ft, farbherd_header_t mhead) {
	if (mhead.frameExtSize)
		if (farbherd_read_buffer(input, ft->frameExtData, mhead.frameExtSize))
			return 1;
	size_t datasize = farbherd_datasize(mhead.imageHead);
	if (datasize)
		if (farbherd_read_buffer(input, ft->deltas, datasize))
			return 1;
	return 0;
}

inline static void farbherd_write_farbherd_frame(FILE * output, farbherd_frame_t * ft, farbherd_header_t mhead) {
	size_t datasize = farbherd_datasize(mhead.imageHead);
	setvbuf(output, NULL, _IOFBF, mhead.frameExtSize + datasize);
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise(fileno(output), 0, mhead.frameExtSize + datasize, POSIX_FADV_SEQUENTIAL);
#endif

	if (mhead.frameExtSize)
		fwrite(ft->frameExtData, mhead.frameExtSize, 1, output);

	if (datasize)
		fwrite(ft->deltas, datasize, 1, output);
}

// Copies source to working, leaving source containing the delta from the old working values to the old source values.
// Useful for encoders.
inline static void farbherd_calc_apply_delta(uint16_t * working, uint16_t * source, size_t datasize) {
	for (size_t n = 0; n < datasize; n += sizeof(uint16_t)) {
		uint16_t value = farbherd_be16toh(*source);
		*source = farbherd_htobe16(value - farbherd_be16toh(*working));
		*working = farbherd_htobe16(value);
		working++;
		source++;
	}
}
// Applies the deltas in source to working, leaving source unmodified.
inline static void farbherd_apply_delta(uint16_t * working, const uint16_t * source, size_t datasize) {
	for (size_t n = 0; n < datasize; n += sizeof(uint16_t)) {
		*working = farbherd_htobe16(farbherd_be16toh(*working) + farbherd_be16toh(*source));
		working++;
		source++;
	}
}

#define FARBHERD_TIMEDIV_TO_MS(time, div) (((time) * 1000) / (div))
