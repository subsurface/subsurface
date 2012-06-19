#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dive.h"
#include "file.h"

#define DON

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

static int show_line(unsigned offset, const unsigned char *data, unsigned size, int show_empty)
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

	if (bits) {
		puts(buffer);
		return 1;
	}
	if (show_empty)
		puts("...");
	return 0;
}

static void cochran_debug_write(const char *filename, const unsigned char *data, unsigned size)
{
	int i, show = 1;

	for (i = 0; i < size; i += 16)
		show = show_line(i, data + i, size - i, show);
}

static void parse_cochran_header(const char *filename,
		const unsigned char *decode, unsigned mod,
		const unsigned char *in, unsigned size)
{
	char *buf = malloc(size);

	/* Do the "null decode" using a one-byte decode array of '\0' */
	partial_decode(0    , 0x0b14, "", 0, 1, in, size, buf);

	/*
	 * The header scrambling is different form the dive
	 * scrambling. Oh yay!
	 */
	partial_decode(0x010e, 0x0b14, decode, 0, mod, in, size, buf);
	partial_decode(0x0b14, 0x1b14, decode, 0, mod, in, size, buf);
	partial_decode(0x1b14, 0x2b14, decode, 0, mod, in, size, buf);
	partial_decode(0x2b14, 0x3b14, decode, 0, mod, in, size, buf);
	partial_decode(0x3b14, 0x5414, decode, 0, mod, in, size, buf);
	partial_decode(0x5414,   size, decode, 0, mod, in, size, buf);

	printf("\n%s, header\n\n", filename);
	cochran_debug_write(filename, buf, size);

	free(buf);
}

/*
 * Cochran export files show that depths seem to be in
 * quarter feet (rounded up to tenths).
 *
 * Temperature seems to be exported in Fahrenheit.
 *
 * Cylinder pressure seems to be in multiples of 4 psi.
 *
 * The data seems to be some byte-stream where the pattern
 * appears to be that the two high bits indicate type of
 * data.
 *
 * For '00', the low six bits seem to be positive
 * values with a distribution towards zero, probably depth
 * deltas. '0 0' exists, but is very rare ("surface"?). 63
 * exists, but is rare.
 *
 * For '01', the low six bits seem to be a signed binary value,
 * with the most common being 0, and 1 and -1 (63) being the
 * next most common values.
 *
 * NOTE! Don's CAN data is different. It shows the reverse pattern
 * for 00 and 01 above: 00 looks like signed data, with 01 looking
 * like unsigned data.
 *
 * For '10', there seems to be another positive value distribution,
 * but unlike '00' the value 0 is common, and I see examples of 63
 * too ("overflow"?) and a spike at '7'.
 *
 * Again, Don's data is different.
 *
 * The values for '11' seem to be some exception case. Possibly
 * overflow handling, possibly warning events. It doesn't have
 * any clear distribution: values 0, 1, 16, 33, 35, 48, 51, 55
 * and 63 are common.
 *
 * For David and Don's data, '01' is the most common, with '00'
 * and '10' not uncommon. '11' is two orders of magnitude less
 * common.
 *
 * For Alex, '00' is the most common, with 01 about a third as
 * common, and 02 a third of that. 11 is least common.
 *
 * There clearly are variations in the format here. And Alex has
 * a different data offset than Don/David too (see the #ifdef DON).
 * Christ. Maybe I've misread the patterns entirely.
 */
static void cochran_profile_write(const unsigned char *buf, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		unsigned char c = buf[i];
		printf("%d %d\n",
			c >> 6, c & 0x3f);
	}
}

static void parse_cochran_dive(const char *filename, int dive,
		const unsigned char *decode, unsigned mod,
		const unsigned char *in, unsigned size)
{
	char *buf = malloc(size);
#ifdef DON
	unsigned int offset = 0x4a14;
#else
	unsigned int offset = 0x4b14;
#endif

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
	partial_decode(0x48ff, offset, decode, 0, mod, in, size, buf);
	partial_decode(offset,   size, decode, 0, mod, in, size, buf);

	printf("\n%s, dive %d\n\n", filename, dive);
	cochran_debug_write(filename, buf, size);
	cochran_profile_write(buf + offset, size - offset);

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

	parse_cochran_header(filename, decode, mod, mem->buffer + 0x40000, dive1 - 0x40000);

	for (i = 0; i < 65534; i++) {
		dive1 = offsets[i];
		dive2 = offsets[i+1];
		if (dive2 < dive1)
			break;
		if (dive2 > mem->size)
			break;
		parse_cochran_dive(filename, i+1, decode, mod, mem->buffer + dive1, dive2 - dive1);
	}

	exit(0);
}
