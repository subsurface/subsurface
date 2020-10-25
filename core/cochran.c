// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dive.h"
#include "file.h"
#include "sample.h"
#include "subsurface-time.h"
#include "units.h"
#include "sha1.h"
#include "gettext.h"
#include "cochran.h"
#include "divelist.h"

#include <libdivecomputer/parser.h>

#define POUND       0.45359237
#define FEET        0.3048
#define INCH        0.0254
#define GRAVITY     9.80665
#define ATM         101325.0
#define BAR         100000.0
#define FSW         (ATM / 33.0)
#define MSW         (BAR / 10.0)
#define PSI         ((POUND * GRAVITY) / (INCH * INCH))

// Some say 0x4a14 and 0x4b14 are the right number for this offset
// This works with CAN files from Analyst 4.01v and computers
// such as Commander, Gemini, EMC-16, and EMC-20H
#define LOG_ENTRY_OFFSET 0x4914

enum cochran_type {
	TYPE_GEMINI,
	TYPE_COMMANDER,
	TYPE_EMC
};

struct config {
	enum cochran_type type;
	unsigned int logbook_size;
	unsigned int sample_size;
} config;


// Convert 4 bytes into an INT
#define array_uint16_le(p) ((unsigned int) (p)[0] \
			    + ((p)[1]<<8) )
#define array_uint32_le(p) ((unsigned int) (p)[0] \
			    + ((p)[1]<<8) + ((p)[2]<<16) \
			    + ((p)[3]<<24))

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
 * 0x40000: byte 0x46
 * 0x40001: "block 0": 256 byte encryption key
 * 0x40101: the random modulus, or length of the key to use
 * 0x40102: block 1: Version and date of Analyst and a feature string identifying
 *          the computer features and the features of the file
 * 0x40138: Computer configuration page 1, 512 bytes
 * 0x40338: Computer configuration page 2, 512 bytes
 * 0x40538: Misc data (tissues) 1500 bytes
 * 0x40b14: Ownership data 512 bytes ???
 *
 * 0x4171c: Ownership data 512 bytes ??? <copy>
 *
 * 0x45415: Time stamp 17 bytes
 * 0x45426: Computer configuration page 1, 512 bytes <copy>
 * 0x45626: Computer configuration page 2, 512 bytes <copy>
 *
 */
static unsigned int partial_decode(unsigned int start, unsigned int end,
				   const unsigned char *decode, unsigned offset, unsigned mod,
				   const unsigned char *buf, unsigned int size, unsigned char *dst)
{
	unsigned i, sum = 0;

	for (i = start; i < end; i++) {
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

#ifdef COCHRAN_DEBUG

#define hexchar(n) ("0123456789abcdef"[(n) & 15])

static int show_line(unsigned offset, const unsigned char *data,
		     unsigned size, int show_empty)
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
		char *hex = buffer + off + 3 * i;
		char *asc = buffer + off + 50 + i;
		unsigned char byte = data[i];

		hex[0] = hexchar(byte >> 4);
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

static void cochran_debug_write(const unsigned char *data, unsigned size)
{
	return;

	int show = 1,  i;
	for (i = 0; i < size; i += 16)
		show = show_line(i, data + i, size - i, show);
}

static void cochran_debug_sample(const char *s, unsigned int sample_cnt)
{
	switch (config.type) {
	case TYPE_GEMINI:
		switch (sample_cnt % 4) {
		case 0:
			printf("Hex: %02x %02x          ", s[0], s[1]);
			break;
		case 1:
			printf("Hex: %02x    %02x       ", s[0], s[1]);
			break;
		case 2:
			printf("Hex: %02x       %02x    ", s[0], s[1]);
			break;
		case 3:
			printf("Hex: %02x          %02x ", s[0], s[1]);
			break;
		}
		break;
	case TYPE_COMMANDER:
		switch (sample_cnt % 2) {
		case 0:
			printf("Hex: %02x %02x    ", s[0], s[1]);
			break;
		case 1:
			printf("Hex: %02x    %02x ", s[0], s[1]);
			break;
		}
		break;
	case TYPE_EMC:
		switch (sample_cnt % 2) {
		case 0:
			printf("Hex: %02x %02x    %02x ", s[0], s[1], s[2]);
			break;
		case 1:
			printf("Hex: %02x    %02x %02x ", s[0], s[1], s[2]);
			break;
		}
		break;
	}

	printf ("%02dh %02dm %02ds: Depth: %-5.2f, ", sample_cnt / 3660,
		(sample_cnt % 3660) / 60, sample_cnt % 60, depth);
}

#endif  // COCHRAN_DEBUG

static void cochran_parse_header(const unsigned char *decode, unsigned mod,
				 const unsigned char *in, unsigned size)
{
	unsigned char *buf = malloc(size);

	/* Do the "null decode" using a one-byte decode array of '\0' */
	/* Copies in plaintext, will be overwritten later */
	partial_decode(0, 0x0102, (const unsigned char *)"", 0, 1, in, size, buf);

	/*
	 * The header scrambling is different form the dive
	 * scrambling. Oh yay!
	 */
	partial_decode(0x0102, 0x010e, decode, 0, mod, in, size, buf);
	partial_decode(0x010e, 0x0b14, decode, 0, mod, in, size, buf);
	partial_decode(0x0b14, 0x1b14, decode, 0, mod, in, size, buf);
	partial_decode(0x1b14, 0x2b14, decode, 0, mod, in, size, buf);
	partial_decode(0x2b14, 0x3b14, decode, 0, mod, in, size, buf);
	partial_decode(0x3b14, 0x5414, decode, 0, mod, in, size, buf);
	partial_decode(0x5414,  size, decode, 0, mod, in, size, buf);

	// Detect log type
	switch (buf[0x133]) {
	case '2':	// Cochran Commander, version II log format
		config.logbook_size = 256;
		if (buf[0x132] == 0x10) {
			config.type = TYPE_GEMINI;
			config.sample_size = 2;	// Gemini with tank PSI samples
		} else  {
			config.type = TYPE_COMMANDER;
			config.sample_size = 2;	// Commander
		}
		break;
	case '3':	// Cochran EMC, version III log format
		config.type = TYPE_EMC;
		config.logbook_size = 512;
		config.sample_size = 3;
		break;
	default:
		printf ("Unknown log format v%c\n", buf[0x137]);
		free(buf);
		exit(1);
		break;
	}

#ifdef COCHRAN_DEBUG
	puts("Header\n======\n\n");
	cochran_debug_write(buf, size);
#endif

	free(buf);
}

/*
* Bytes expected after a pre-dive event code
*/
static int cochran_predive_event_bytes(unsigned char code)
{
	int x = 0;
	int cmdr_event_bytes[15][2] = {{0x00, 16}, {0x01, 20}, {0x02, 17},
				       {0x03, 16}, {0x06, 18}, {0x07, 18},
				       {0x08, 18}, {0x09, 18}, {0x0a, 18},
				       {0x0b, 18}, {0x0c, 18}, {0x0d, 18},
				       {0x0e, 18}, {0x10, 20},
				       {-1,  0}};
	int emc_event_bytes[15][2] =  {{0x00, 18}, {0x01, 22}, {0x02, 19},
				       {0x03, 18}, {0x06, 20}, {0x07, 20},
				       {0x0a, 20}, {0x0b, 20}, {0x0f, 18},
				       {0x10, 20},
				       {-1,  0}};

	switch (config.type) {
	case TYPE_GEMINI:
	case TYPE_COMMANDER:
		while (cmdr_event_bytes[x][0] != code && cmdr_event_bytes[x][0] != -1)
			x++;
		return cmdr_event_bytes[x][1];
		break;
	case TYPE_EMC:
		while (emc_event_bytes[x][0] != code && emc_event_bytes[x][0] != -1)
			x++;
		return emc_event_bytes[x][1];
		break;
	}

	return 0;
}

int cochran_dive_event_bytes(unsigned char event)
{
	return (event == 0xAD || event == 0xAB) ? 4 : 0;
}

static void cochran_dive_event(struct divecomputer *dc, const unsigned char *s,
			       unsigned int seconds, unsigned int *in_deco,
			       unsigned int *deco_ceiling, unsigned int *deco_time)
{
	switch (s[0]) {
	case 0xC5:	// Deco obligation begins
		*in_deco = 1;
		add_event(dc, seconds, SAMPLE_EVENT_DECOSTOP,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "deco stop"));
		break;
	case 0xDB:	// Deco obligation ends
		*in_deco = 0;
		add_event(dc, seconds, SAMPLE_EVENT_DECOSTOP,
			SAMPLE_FLAGS_END, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "deco stop"));
		break;
	case 0xAD:	// Raise deco ceiling 10 ft
		*deco_ceiling -= 10; // ft
		*deco_time = (array_uint16_le(s + 3) + 1) * 60;
		break;
	case 0xAB:	// Lower deco ceiling 10 ft
		*deco_ceiling += 10;	// ft
		*deco_time = (array_uint16_le(s + 3) + 1) * 60;
		break;
	case 0xA8:	// Entered Post Dive interval mode (surfaced)
		break;
	case 0xA9:	// Exited PDI mode (re-submierged)
		break;
	case 0xBD:	// Switched to normal PO2 setting
		break;
	case 0xC0:	// Switched to FO2 21% mode (generally upon surface)
		break;
	case 0xC1:	// "Ascent rate alarm
		add_event(dc, seconds, SAMPLE_EVENT_ASCENT,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "ascent"));
		break;
	case 0xC2:	// Low battery warning
#ifdef SAMPLE_EVENT_BATTERY
		add_event(dc, seconds, SAMPLE_EVENT_BATTERY,
			SAMPLE_FLAGS_NONE, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "battery"));
#endif
		break;
	case 0xC3:	// CNS warning
		add_event(dc, seconds, SAMPLE_EVENT_OLF,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "OLF"));
		break;
	case 0xC4:	// Depth alarm begin
		add_event(dc, seconds, SAMPLE_EVENT_MAXDEPTH,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "maxdepth"));
		break;
	case 0xC8:	// PPO2 alarm begin
		add_event(dc, seconds, SAMPLE_EVENT_PO2,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "pO₂"));
		break;
	case 0xCC:	// Low cylinder 1 pressure";
		break;
	case 0xCD:	// Switch to deco blend setting
		add_event(dc, seconds, SAMPLE_EVENT_GASCHANGE,
			SAMPLE_FLAGS_NONE, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "gaschange"));
		break;
	case 0xCE:	// NDL alarm begin
		add_event(dc, seconds, SAMPLE_EVENT_RBT,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "rbt"));
		break;
	case 0xD0:	// Breathing rate alarm begin
		break;
	case 0xD3:	// Low gas 1 flow rate alarm begin";
		break;
	case 0xD6:	// Ceiling alarm begin
		add_event(dc, seconds, SAMPLE_EVENT_CEILING,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "ceiling"));
		break;
	case 0xD8:	// End decompression mode
		*in_deco = 0;
		add_event(dc, seconds, SAMPLE_EVENT_DECOSTOP,
			SAMPLE_FLAGS_END, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "deco stop"));
		break;
	case 0xE1:	// Ascent alarm end
		add_event(dc, seconds, SAMPLE_EVENT_ASCENT,
			SAMPLE_FLAGS_END, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "ascent"));
		break;
	case 0xE2:	// Low transmitter battery alarm
		add_event(dc, seconds, SAMPLE_EVENT_TRANSMITTER,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "transmitter"));
		break;
	case 0xE3:	// Switch to FO2 mode
		break;
	case 0xE5:	// Switched to PO2 mode
		break;
	case 0xE8:	// PO2 too low alarm
		add_event(dc, seconds, SAMPLE_EVENT_PO2,
			SAMPLE_FLAGS_BEGIN, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "pO₂"));
		break;
	case 0xEE:	// NDL alarm end
		add_event(dc, seconds, SAMPLE_EVENT_RBT,
			SAMPLE_FLAGS_END, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "rbt"));
		break;
	case 0xEF:	// Switch to blend 2
		add_event(dc, seconds, SAMPLE_EVENT_GASCHANGE,
			SAMPLE_FLAGS_NONE, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "gaschange"));
		break;
	case 0xF0:	// Breathing rate alarm end
		break;
	case 0xF3:	// Switch to blend 1 (often at dive start)
		add_event(dc, seconds, SAMPLE_EVENT_GASCHANGE,
			SAMPLE_FLAGS_NONE, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "gaschange"));
		break;
	case 0xF6:	// Ceiling alarm end
		add_event(dc, seconds, SAMPLE_EVENT_CEILING,
			SAMPLE_FLAGS_END, 0,
			QT_TRANSLATE_NOOP("gettextFromC", "ceiling"));
		break;
	default:
		break;
	}
}

/*
* Parse sample data, extract events and build a dive
*/
static void cochran_parse_samples(struct dive *dive, const unsigned char *log,
				  const unsigned char *samples, unsigned int size,
				  unsigned int *duration, double *max_depth,
				  double *avg_depth, double *min_temp)
{
	const unsigned char *s;
	unsigned int offset = 0, profile_period = 1, sample_cnt = 0;
	double depth = 0, temp = 0, depth_sample = 0, psi = 0, sgc_rate = 0;
	int ascent_rate = 0;
	unsigned int ndl = 0;
	unsigned int in_deco = 0, deco_ceiling = 0, deco_time = 0;

	struct divecomputer *dc = &dive->dc;
	struct sample *sample;

	// Initialize stat variables
	*max_depth = 0, *avg_depth = 0, *min_temp = 0xFF;

	// Get starting depth and temp (tank PSI???)
	switch (config.type) {
	case TYPE_GEMINI:
		depth = (double) (log[CMD_START_DEPTH]
			+ log[CMD_START_DEPTH + 1] * 256) / 4;
		temp = log[CMD_START_TEMP];
		psi = log[CMD_START_PSI] + log[CMD_START_PSI + 1] * 256;
		sgc_rate = (double)(log[CMD_START_SGC]
			+ log[CMD_START_SGC + 1] * 256) / 2;
		profile_period = log[CMD_PROFILE_PERIOD];
		break;
	case TYPE_COMMANDER:
		depth = (double) (log[CMD_START_DEPTH]
			+ log[CMD_START_DEPTH + 1] * 256) / 4;
		temp = log[CMD_START_TEMP];
		profile_period = log[CMD_PROFILE_PERIOD];
		break;

	case TYPE_EMC:
		depth = (double) log [EMC_START_DEPTH] / 256
			+ log[EMC_START_DEPTH + 1];
		temp = log[EMC_START_TEMP];
		profile_period = log[EMC_PROFILE_PERIOD];
		break;
	}

	// Skip past pre-dive events
	unsigned int x = 0;
	unsigned int c;
	while (x < size && (samples[x] & 0x80) == 0 && samples[x] != 0x40) {
		c = cochran_predive_event_bytes(samples[x]) + 1;
#ifdef COCHRAN_DEBUG
		printf("Predive event: ");
		for (unsigned int y = 0; y < c && x + y < size; y++) printf("%02x ", samples[x + y]);
		putchar('\n');
#endif
			x += c;
	}

	// Now process samples
	offset = x;
	while (offset + config.sample_size < size) {
		s = samples + offset;

		// Start with an empty sample
		sample = prepare_sample(dc);
		sample->time.seconds = sample_cnt * profile_period;

		// Check for event
		if (s[0] & 0x80) {
			cochran_dive_event(dc, s, sample_cnt * profile_period, &in_deco, &deco_ceiling, &deco_time);
			offset += cochran_dive_event_bytes(s[0]) + 1;
			continue;
		}

		// Depth is in every sample
		depth_sample = (double)(s[0] & 0x3F) / 4 * (s[0] & 0x40 ? -1 : 1);
		depth += depth_sample;

#ifdef COCHRAN_DEBUG
		cochran_debug_sample(s, sample_cnt);
#endif

		switch (config.type) {
		case TYPE_COMMANDER:
			switch (sample_cnt % 2) {
			case 0:	// Ascent rate
				ascent_rate = (s[1] & 0x7f) * (s[1] & 0x80 ? 1: -1);
				break;
			case 1:	// Temperature
				temp = s[1] / 2 + 20;
				break;
			}
			break;
		case TYPE_GEMINI:
			// Gemini with tank pressure and SAC rate.
			switch (sample_cnt % 4) {
			case 0:	// Ascent rate
				ascent_rate = (s[1] & 0x7f) * (s[1] & 0x80 ? 1 : -1);
				break;
			case 2:	// PSI change
				psi -= (double)(s[1] & 0x7f) * (s[1] & 0x80 ? 1 : -1) / 4;
				break;
			case 1:	// SGC rate
				sgc_rate -= (double)(s[1] & 0x7f) * (s[1] & 0x80 ? 1 : -1) / 2;
				break;
			case 3:	// Temperature
				temp = (double)s[1] / 2 + 20;
				break;
			}
			break;
		case TYPE_EMC:
			switch (sample_cnt % 2) {
			case 0:	// Ascent rate
				ascent_rate = (s[1] & 0x7f) * (s[1] & 0x80 ? 1: -1);
				break;
			case 1:	// Temperature
				temp = (double)s[1] / 2 + 20;
				break;
			}
			// Get NDL and deco information
			switch (sample_cnt % 24) {
			case 20:
				if (offset + 5 < size) {
					if (in_deco) {
						// Fist stop time
						//first_deco_time = (s[2] + s[5] * 256 + 1) * 60; // seconds
						ndl = 0;
					} else {
						// NDL
						ndl = (s[2] + s[5] * 256 + 1) * 60; // seconds
						deco_time = 0;
					}
				}
				break;
			case 22:
				if (offset + 5 < size) {
					if (in_deco) {
						// Total stop time
						deco_time = (s[2] + s[5] * 256 + 1) * 60; // seconds
						ndl = 0;
					}
				}
				break;
			}
		}

		// Track dive stats
		if (depth > *max_depth) *max_depth = depth;
		if (temp < *min_temp) *min_temp = temp;
		*avg_depth = (*avg_depth * sample_cnt + depth) / (sample_cnt + 1);

		sample->depth.mm = lrint(depth * FEET * 1000);
		sample->ndl.seconds = ndl;
		sample->in_deco = in_deco;
		sample->stoptime.seconds = deco_time;
		sample->stopdepth.mm = lrint(deco_ceiling * FEET * 1000);
		sample->temperature.mkelvin = F_to_mkelvin(temp);
		sample->sensor[0] = 0;
		sample->pressure[0].mbar = lrint(psi * PSI / 100);

		finish_sample(dc);

		offset += config.sample_size;
		sample_cnt++;
	}
	UNUSED(ascent_rate); // mark the variable as unused

	if (sample_cnt > 0)
		*duration = sample_cnt * profile_period - 1;
}

static void cochran_parse_dive(const unsigned char *decode, unsigned mod,
			       const unsigned char *in, unsigned size,
			       struct dive_table *table)
{
	unsigned char *buf = malloc(size);
	struct dive *dive;
	struct divecomputer *dc;
	struct tm tm = {0};
	uint32_t csum[5];

	double max_depth, avg_depth, min_temp;
	unsigned int duration = 0, corrupt_dive = 0;

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
	partial_decode(0, 0x0fff, decode, 1, mod, in, size, buf);
	partial_decode(0x0fff, 0x1fff, decode, 0, mod, in, size, buf);
	partial_decode(0x1fff, 0x2fff, decode, 0, mod, in, size, buf);
	partial_decode(0x2fff, 0x48ff, decode, 0, mod, in, size, buf);

	/*
	 * This is not all the descrambling you need - the above are just
	 * what appears to be the fixed-size blocks. The rest is also
	 * scrambled, but there seems to be size differences in the data,
	 * so this just descrambles part of it:
	 */

	if (size < 0x4914 + config.logbook_size) {
		// Analyst calls this a "Corrupt Beginning Summary"
		free(buf);
		return;
	}

	// Decode log entry (512 bytes + random prefix)
	partial_decode(0x48ff, 0x4914 + config.logbook_size, decode,
		0, mod, in, size, buf);

	unsigned int sample_size = size - 0x4914 - config.logbook_size;
	int g;
	unsigned int sample_pre_offset = 0, sample_end_offset = 0;

	// Decode sample data
	partial_decode(0x4914 + config.logbook_size, size, decode,
		0, mod, in, size, buf);

#ifdef COCHRAN_DEBUG
	// Display pre-logbook data
	puts("\nPre Logbook Data\n");
	cochran_debug_write(buf, 0x4914);

	// Display log book
	puts("\nLogbook Data\n");
	cochran_debug_write(buf + 0x4914,  config.logbook_size + 0x400);

	// Display sample data
	puts("\nSample Data\n");
#endif

	dive = alloc_dive();
	dc = &dive->dc;

	unsigned char *log = (buf + 0x4914);

	switch (config.type) {
	case TYPE_GEMINI:
	case TYPE_COMMANDER:
		if (config.type == TYPE_GEMINI) {
			cylinder_t cyl = empty_cylinder;
			dc->model = "Gemini";
			dc->deviceid = buf[0x18c] * 256 + buf[0x18d];	// serial no
			fill_default_cylinder(dive, &cyl);
			cyl.gasmix.o2.permille = (log[CMD_O2_PERCENT] / 256
				+ log[CMD_O2_PERCENT + 1]) * 10;
			cyl.gasmix.he.permille = 0;
			add_cylinder(&dive->cylinders, 0, cyl);
		} else {
			dc->model = "Commander";
			dc->deviceid = array_uint32_le(buf + 0x31e);	// serial no
			for (g = 0; g < 2; g++) {
				cylinder_t cyl = empty_cylinder;
				fill_default_cylinder(dive, &cyl);
				cyl.gasmix.o2.permille = (log[CMD_O2_PERCENT + g * 2] / 256
					+ log[CMD_O2_PERCENT + g * 2 + 1]) * 10;
				cyl.gasmix.he.permille = 0;
				add_cylinder(&dive->cylinders, g, cyl);
			}
		}

		tm.tm_year = log[CMD_YEAR];
		tm.tm_mon = log[CMD_MON] - 1;
		tm.tm_mday = log[CMD_DAY];
		tm.tm_hour = log[CMD_HOUR];
		tm.tm_min = log[CMD_MIN];
		tm.tm_sec = log[CMD_SEC];
		tm.tm_isdst = -1;

		dive->when = dc->when = utc_mktime(&tm);
		dive->number = log[CMD_NUMBER] + log[CMD_NUMBER + 1] * 256 + 1;
		dc->duration.seconds = (log[CMD_BT] + log[CMD_BT + 1] * 256) * 60;
		dc->surfacetime.seconds = (log[CMD_SIT] + log[CMD_SIT + 1] * 256) * 60;
		dc->maxdepth.mm = lrint((log[CMD_MAX_DEPTH] +
			log[CMD_MAX_DEPTH + 1] * 256) / 4 * FEET * 1000);
		dc->meandepth.mm = lrint((log[CMD_AVG_DEPTH] +
			log[CMD_AVG_DEPTH + 1] * 256) / 4 * FEET * 1000);
		dc->watertemp.mkelvin = F_to_mkelvin(log[CMD_MIN_TEMP]);
		dc->surface_pressure.mbar = lrint(ATM / BAR * pow(1 - 0.0000225577
			* (double) log[CMD_ALTITUDE] * 250 * FEET, 5.25588) * 1000);
		dc->salinity = 10000 + 150 * log[CMD_WATER_CONDUCTIVITY];

		SHA1(log + CMD_NUMBER, 2, (unsigned char *)csum);
		dc->diveid = csum[0];

		if (log[CMD_MAX_DEPTH] == 0xff && log[CMD_MAX_DEPTH + 1] == 0xff)
			corrupt_dive = 1;

		sample_pre_offset = array_uint32_le(log + CMD_PREDIVE_OFFSET);
		sample_end_offset = array_uint32_le(log + CMD_END_OFFSET);

		break;
	case TYPE_EMC:
		dc->model = "EMC";
		dc->deviceid = array_uint32_le(buf + 0x31e);	// serial no
		for (g = 0; g < 4; g++) {
			cylinder_t cyl = empty_cylinder;
			fill_default_cylinder(dive, &cyl);
			cyl.gasmix.o2.permille =
				(log[EMC_O2_PERCENT + g * 2] / 256
				+ log[EMC_O2_PERCENT + g * 2 + 1]) * 10;
			cyl.gasmix.he.permille =
				(log[EMC_HE_PERCENT + g * 2] / 256
				+ log[EMC_HE_PERCENT + g * 2 + 1]) * 10;
			add_cylinder(&dive->cylinders, g, cyl);
		}

		tm.tm_year = log[EMC_YEAR];
		tm.tm_mon = log[EMC_MON] - 1;
		tm.tm_mday = log[EMC_DAY];
		tm.tm_hour = log[EMC_HOUR];
		tm.tm_min = log[EMC_MIN];
		tm.tm_sec = log[EMC_SEC];
		tm.tm_isdst = -1;

		dive->when = dc->when = utc_mktime(&tm);
		dive->number = log[EMC_NUMBER] + log[EMC_NUMBER + 1] * 256 + 1;
		dc->duration.seconds = (log[EMC_BT] + log[EMC_BT + 1] * 256) * 60;
		dc->surfacetime.seconds = (log[EMC_SIT] + log[EMC_SIT + 1] * 256) * 60;
		dc->maxdepth.mm = lrint((log[EMC_MAX_DEPTH] +
			log[EMC_MAX_DEPTH + 1] * 256) / 4 * FEET * 1000);
		dc->meandepth.mm = lrint((log[EMC_AVG_DEPTH] +
			log[EMC_AVG_DEPTH + 1] * 256) / 4 * FEET * 1000);
		dc->watertemp.mkelvin = F_to_mkelvin(log[EMC_MIN_TEMP]);
		dc->surface_pressure.mbar = lrint(ATM / BAR * pow(1 - 0.0000225577
			* (double) log[EMC_ALTITUDE] * 250 * FEET, 5.25588) * 1000);
		dc->salinity = 10000 + 150 * (log[EMC_WATER_CONDUCTIVITY] & 0x3);

		SHA1(log + EMC_NUMBER, 2, (unsigned char *)csum);
		dc->diveid = csum[0];

		if (log[EMC_MAX_DEPTH] == 0xff && log[EMC_MAX_DEPTH + 1] == 0xff)
			corrupt_dive = 1;

		sample_pre_offset = array_uint32_le(log + EMC_PREDIVE_OFFSET);
		sample_end_offset = array_uint32_le(log + EMC_END_OFFSET);

		break;
	}

	// Use the log information to determine actual profile sample size
	// Otherwise we will get surface time at end of dive.
	if (sample_pre_offset < sample_end_offset && sample_end_offset != 0xffffffff)
		sample_size = sample_end_offset - sample_pre_offset;

	cochran_parse_samples(dive, buf + 0x4914, buf + 0x4914
		+ config.logbook_size, sample_size,
		&duration, &max_depth, &avg_depth, &min_temp);

	// Check for corrupt dive
	if (corrupt_dive) {
		dc->maxdepth.mm = lrint(max_depth * FEET * 1000);
		dc->meandepth.mm = lrint(avg_depth * FEET * 1000);
		dc->watertemp.mkelvin = F_to_mkelvin(min_temp);
		dc->duration.seconds = duration;
	}

	record_dive_to_table(dive, table);

	free(buf);
}

int try_to_open_cochran(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites)
{
	UNUSED(filename);
	UNUSED(trips);
	UNUSED(sites);
	unsigned int i;
	unsigned int mod;
	unsigned int *offsets, dive1, dive2;
	unsigned char *decode = mem->buffer + 0x40001;

	if (mem->size < 0x40000)
		return 0;

	offsets = (unsigned int *) mem->buffer;
	dive1 = offsets[0];
	dive2 = offsets[1];

	if (dive1 < 0x40000 || dive2 < dive1 || dive2 > mem->size)
		return 0;

	mod = decode[0x100] + 1;
	cochran_parse_header(decode, mod, mem->buffer + 0x40000, dive1 - 0x40000);

	// Decode each dive
	for (i = 0; i < 65534; i++) {
		dive1 = offsets[i];
		dive2 = offsets[i + 1];
		if (dive2 < dive1)
			break;
		if (dive2 > mem->size)
			break;

		cochran_parse_dive(decode, mod, mem->buffer + dive1,
						dive2 - dive1, table);
	}

	return 1; // no further processing needed
}
