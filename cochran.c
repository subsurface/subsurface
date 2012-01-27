#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dive.h"
#include "file.h"

/*
 * The Cochran file format is designed to be annoying to read. It's roughly:
 *
 * 0x00000: room for 65534 4-byte words, giving the starting offsets
 *   of the dives themselves.
 *
 * 0x3fff8: the size of the file + 1
 * 0x3ffff: 0 (high 32 bits of filesize? Bogus: the offsets into the file
 *   are 32-bit, so it can't be a large file anyway)
 *
 * 0x40000: "block 0": the decoding block. The first byte is some random
 *   value (0x46 in the files I have access to), the next 200+ bytes or so
 *   are the "scrambling array" that needs to be added into the file
 *   contents to make sense of them.
 *
 * The descrambling array seems to be of some random size which is likely
 * determinable from the array somehow, the two test files I have it as
 * 230 bytes and 234 bytes respectively.
 */
static unsigned int partial_decode(unsigned int start, unsigned int end,
		const unsigned char *decode, unsigned offset, unsigned mod,
		const unsigned char *buf, unsigned int size, unsigned char *dst)
{
	unsigned i, sum = 0;

	for (i = start ; i < end; i++) {
		unsigned char d = decode[offset++];
		if (i >= size)
			break;
		if (offset == mod)
			offset = 0;
		d += buf[i];
		if (dst)
			dst[i] = d;
		sum += d;
	}
	return sum;
}

/*
 * The decode buffer size can be figured out by simply trying our the
 * decode: we expect that the scrambled contents are largely random, and
 * thus tend to have half the bits set. Summing over the bytes is going
 * to give an average of 0x80 per byte.
 *
 * The decoded array is mostly full of zeroes, so the sum is lower.
 *
 * Works for me.
 */
static int figure_out_modulus(const unsigned char *decode, const unsigned char *dive, unsigned int size)
{
	int mod, best = -1;
	unsigned int min = ~0u;

	if (size < 0x1000)
		return best;

	for (mod = 50; mod < 300; mod++) {
		unsigned int sum;

		sum = partial_decode(0, 0x0fff, decode, 1, mod, dive, size, NULL);
		if (sum < min) {
			min = sum;
			best = mod;
		}
	}
	return best;
}

#define hexchar(n) ("0123456789abcdef"[(n)&15])

static void show_line(unsigned offset, const unsigned char *data, unsigned size)
{
	unsigned char bits;
	int i, off;
	char buffer[120];

	if (size > 16)
		size = 16;

	bits = 0;
	memset(buffer, ' ', sizeof(buffer));
	off = sprintf(buffer, "%06x ", offset);
	for (i = 0; i < size; i++) {
		char *hex = buffer + off + 3*i;
		char *asc = buffer + off + 50 + i;
		unsigned char byte = data[i];

		hex[0] = hexchar(byte>>4);
		hex[1] = hexchar(byte);
		bits |= byte;
		if (byte < 32 || byte > 126)
			byte = '.';
		asc[0] = byte;
		asc[1] = 0;
	}

	if (bits)
		puts(buffer);
}

static void cochran_debug_write(const char *filename, int dive, const unsigned char *data, unsigned size)
{
	int i;
	printf("\n%s, dive %d\n\n", filename, dive);

	for (i = 0; i < size; i += 16) {
		show_line(i, data + i, size - i);
	}
}

static void parse_cochran_dive(const char *filename, int dive,
		const unsigned char *decode, unsigned mod,
		const unsigned char *in, unsigned size)
{
	char *buf = malloc(size);

	/*
	 * The scrambling has odd boundaries. I think the boundaries
	 * match some data structure size, but I don't know. They were
	 * discovered the same way we dynamically discover the decode
	 * size: automatically looking for least random output.
	 *
	 * The boundaries are also this confused "off-by-one" thing,
	 * the same way the file size is off by one. It's as if the
	 * cochran software forgot to write one byte at the beginning.
	 */
	partial_decode(0     , 0x0fff, decode, 1, mod, in, size, buf);
	partial_decode(0x0fff, 0x1fff, decode, 0, mod, in, size, buf);
	partial_decode(0x1fff, 0x2fff, decode, 0, mod, in, size, buf);
	partial_decode(0x2fff, 0x48ff, decode, 0, mod, in, size, buf);

	/*
	 * This is not all the descrambling you need - the above are just
	 * what appears to be the fixed-size blocks. The rest is also
	 * scrambled, but there seems to be size differences in the data,
	 * so this just descrambles part of it:
	 */
	partial_decode(0x48ff, 0x4a14, decode, 0, mod, in, size, buf);
	partial_decode(0x4a14, 0xc9bd, decode, 0, mod, in, size, buf);
	partial_decode(0xc9bd,   size, decode, 0, mod, in, size, buf);

	cochran_debug_write(filename, dive, buf, size);

	free(buf);
}

int try_to_open_cochran(const char *filename, struct memblock *mem, GError **error)
{
	unsigned int i;
	unsigned int mod;
	unsigned int *offsets, dive1, dive2;
	unsigned char *decode = mem->buffer + 0x40001;

	if (mem->size < 0x40000)
		return 0;
	offsets = mem->buffer;
	dive1 = offsets[0];
	dive2 = offsets[1];
	if (dive1 < 0x40000 || dive2 < dive1 || dive2 > mem->size)
		return 0;

	mod = figure_out_modulus(decode, mem->buffer + dive1, dive2 - dive1);

	for (i = 0; i < 65534; i++) {
		dive1 = offsets[i];
		dive2 = offsets[i+1];
		if (dive2 < dive1)
			break;
		if (dive2 > mem->size)
			break;
		parse_cochran_dive(filename, i, decode, mod, mem->buffer + dive1, dive2 - dive1);
	}

	exit(0);
}
