#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "datatrak.h"
#include "dive.h"
#include "units.h"
#include "device.h"
#include "gettext.h"

extern struct sample *add_sample(struct sample *sample, int time, struct divecomputer *dc);

unsigned char lector_bytes[2], lector_word[4], tmp_1byte, *byte;
unsigned int tmp_2bytes;
char is_nitrox, is_O2, is_SCR;
unsigned long tmp_4bytes;

static unsigned int two_bytes_to_int(unsigned char x, unsigned char y)
{
	return (x << 8) + y;
}

static unsigned long four_bytes_to_long(unsigned char x, unsigned char y, unsigned char z, unsigned char t)
{
	return ((long)x << 24) + ((long)y << 16) + ((long)z << 8) + (long)t;
}

static unsigned char *byte_to_bits(unsigned char byte)
{
	unsigned char i, *bits = (unsigned char *)malloc(8);

	for (i = 0; i < 8; i++)
		bits[i] = byte & (1 << i);
	return bits;
}

/*
 * Datatrak stores the date in days since 01-01-1600, while Subsurface uses
 * time_t (seconds since 00:00 01-01-1970). Function substracts
 * (1970 - 1600) * 365,2425 = 135139,725  to our date variable, getting the
 * days since Epoch.
 */
static time_t date_time_to_ssrfc(unsigned long date, int time)
{
	time_t tmp;
	tmp = (date - 135140) * 86400 + time * 60;
	return tmp;
}

static unsigned char to_8859(unsigned char char_cp850)
{
	static const unsigned char char_8859[46] = { 0xc7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7,
						     0xea, 0xeb, 0xe8, 0xef, 0xee, 0xec, 0xc4, 0xc5,
						     0xc9, 0xe6, 0xc6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9,
						     0xff, 0xd6, 0xdc, 0xf8, 0xa3, 0xd8, 0xd7, 0x66,
						     0xe1, 0xed, 0xf3, 0xfa, 0xf1, 0xd1, 0xaa, 0xba,
						     0xbf, 0xae, 0xac, 0xbd, 0xbc, 0xa1 };
	return char_8859[char_cp850 - 0x80];
}

static char *to_utf8(unsigned char *in_string)
{
	int outlen, inlen, i = 0, j = 0;
	inlen = strlen((char *)in_string);
	outlen = inlen * 2 + 1;

	char *out_string = calloc(outlen, 1);
	for (i = 0; i < inlen; i++) {
		if (in_string[i] < 127)
			out_string[j] = in_string[i];
		else {
			if (in_string[i] > 127 && in_string[i] <= 173)
				in_string[i] = to_8859(in_string[i]);
			out_string[j] = (in_string[i] >> 6) | 0xC0;
			j++;
			out_string[j] = (in_string[i] & 0x3F) | 0x80;
		}
		j++;
	}
	out_string[j + 1] = '\0';
	return out_string;
}

/*
 * Subsurface sample structure doesn't support the flags and alarms in the dt .log
 * so will treat them as dc events.
 */
static struct sample *dtrak_profile(struct dive *dt_dive, FILE *archivo)
{
	int i, j = 1, interval, o2percent = dt_dive->cylinder[0].gasmix.o2.permille / 10;
	struct sample *sample = dt_dive->dc.sample;
	struct divecomputer *dc = &dt_dive->dc;

	for (i = 1; i <= dt_dive->dc.alloc_samples; i++) {
		if (fread(&lector_bytes, 1, 2, archivo) != 2)
			return sample;
		interval= 20 * (i + 1);
		sample = add_sample(sample, interval, dc);
		sample->depth.mm = (two_bytes_to_int(lector_bytes[0], lector_bytes[1]) & 0xFFC0) * 1000 / 410;
		byte = byte_to_bits(two_bytes_to_int(lector_bytes[0], lector_bytes[1]) & 0x003F);
		if (byte[0] != 0)
			sample->in_deco = true;
		else
			sample->in_deco = false;
		if (byte[1] != 0)
			add_event(dc, sample->time.seconds, 0, 0, 0, "rbt");
		if (byte[2] != 0)
			add_event(dc, sample->time.seconds, 0, 0, 0, "ascent");
		if (byte[3] != 0)
			add_event(dc, sample->time.seconds, 0, 0, 0, "ceiling");
		if (byte[4] != 0)
			add_event(dc, sample->time.seconds, 0, 0, 0, "workload");
		if (byte[5] != 0)
			add_event(dc, sample->time.seconds, 0, 0, 0, "transmitter");
		if (j == 3) {
			read_bytes(1);
			if (is_O2) {
				read_bytes(1);
				o2percent = tmp_1byte;
			}
			j = 0;
		}
		free(byte);

		// In commit 5f44fdd setpoint replaced po2, so although this is not necesarily CCR dive ...
		if (is_O2)
			sample->setpoint.mbar = calculate_depth_to_mbar(sample->depth.mm, dt_dive->surface_pressure, 0) * o2percent / 100;
		j++;
	}
bail:
	return sample;
}

/*
 * Reads the header of a file and returns the header struct
 * If it's not a DATATRAK file returns header zero initalized
 */
static dtrakheader read_file_header(FILE *archivo)
{
	dtrakheader fileheader = { 0 };
	const short headerbytes = 12;
	unsigned char *lector = (unsigned char *)malloc(headerbytes);

	if (fread(lector, 1, headerbytes, archivo) != headerbytes) {
		free(lector);
		return fileheader;
	}
	if (two_bytes_to_int(lector[0], lector[1]) != 0xA100) {
		report_error(translate("gettextFromC", "Error: the file does not appear to be a DATATRAK divelog"));
		free(lector);
		return fileheader;
	}
	fileheader.header = (lector[0] << 8) + lector[1];
	fileheader.dc_serial_1 = two_bytes_to_int(lector[2], lector[3]);
	fileheader.dc_serial_2 = two_bytes_to_int(lector[4], lector[5]);
	fileheader.divesNum = two_bytes_to_int(lector[7], lector[6]);
	free(lector);
	return fileheader;
}

#define CHECK(_func, _val) if ((_func) != (_val)) goto bail

/*
 * Parses the dive extracting its data and filling a subsurface's dive structure
 */
bool dt_dive_parser(FILE *archivo, struct dive *dt_dive)
{
	unsigned char n;
	int  profile_length;
	char *tmp_notes_str = NULL;
	unsigned char *tmp_string1 = NULL,
		      *locality = NULL,
		      *dive_point = NULL;
	char buffer[1024];
	struct divecomputer *dc = &dt_dive->dc;

	is_nitrox = is_O2 = is_SCR = 0;

	/*
	 * Parse byte to byte till next dive entry
	 */
	n = 0;
	CHECK(fread(&lector_bytes[n], 1, 1, archivo), 1);
	while (lector_bytes[n] != 0xA0)
		CHECK(fread(&lector_bytes[n], 1, 1, archivo), 1);

	/*
	 * Found dive header 0xA000, verify second byte
	 */
	CHECK(fread(&lector_bytes[n+1], 1, 1, archivo), 1);
	if (two_bytes_to_int(lector_bytes[0], lector_bytes[1]) != 0xA000) {
		printf("Error: byte = %4x\n", two_bytes_to_int(lector_bytes[0], lector_bytes[1]));
		return false;
	}

	/*
	 * Begin parsing
	 * First, Date of dive, 4 bytes
	 */
	read_bytes(4);


	/*
	 * Next, Time in minutes since 00:00
	 */
	read_bytes(2);

	dt_dive->dc.when = dt_dive->when = (timestamp_t)date_time_to_ssrfc(tmp_4bytes, tmp_2bytes);

	/*
	 * Now, Locality, 1st byte is long of string, rest is string
	 */
	read_bytes(1);
	read_string(locality);

	/*
	 * Next, Dive point, defined as Locality
	 */
	read_bytes(1);
	read_string(dive_point);

	/*
	 * Subsurface only have a location variable, so we have to merge DTrak's
	 * Locality and Dive points.
	 */
	snprintf(buffer, sizeof(buffer), "%s, %s", locality, dive_point);
	dt_dive->dive_site_uuid = get_dive_site_uuid_by_name(buffer, NULL);
	if (dt_dive->dive_site_uuid == 0)
		dt_dive->dive_site_uuid = create_dive_site(buffer, dt_dive->when);
	free(locality);
	free(dive_point);

	/*
	 * Altitude. Don't exist in Subsurface, the equivalent would be
	 * surface air pressure which can, be calculated from altitude.
	 * As dtrak registers altitude intervals, we, arbitrarily, choose
	 * the lower altitude/pressure equivalence for each segment. So
	 *
	 * Datatrak table            *  Conversion formula:
	 *                           *
	 * byte = 1   0 - 700 m      *  P = P0 * exp(-(g * M * h ) / (R * T0))
	 * byte = 2   700 - 1700m    *  P0 = sealevel pressure = 101325 Pa
	 * byte = 3   1700 - 2700 m  *  g = grav. acceleration = 9,80665 m/s²
	 * byte = 4   2700 -  *   m  *  M = molar mass (dry air) = 0,0289644 Kg/mol
	 *                           *  h = altitude over sea level (m)
	 *                           *  R = Universal gas constant = 8,31447 J/(mol*K)
	 *                           *  T0 = sea level standard temperature = 288,15 K
	 */
	read_bytes(1);
	switch (tmp_1byte) {
		case 1:
			dt_dive->dc.surface_pressure.mbar = 1013;
			break;
		case 2:
			dt_dive->dc.surface_pressure.mbar = 932;
			break;
		case 3:
			dt_dive->dc.surface_pressure.mbar = 828;
			break;
		case 4:
			dt_dive->dc.surface_pressure.mbar = 735;
			break;
		default:
			dt_dive->dc.surface_pressure.mbar = 1013;
	}

	/*
	 * Interval (minutes)
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->dc.surfacetime.seconds = (uint32_t) tmp_2bytes * 60;

	/*
	 * Weather, values table, 0 to 6
	 * Subsurface don't have this record but we can use tags
	 */
	dt_dive->tag_list = NULL;
	read_bytes(1);
	switch (tmp_1byte) {
		case 1:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "clear")));
			break;
		case 2:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "misty")));
			break;
		case 3:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "fog")));
			break;
		case 4:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "rain")));
			break;
		case 5:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "storm")));
			break;
		case 6:
			taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "snow")));
			break;
		default:
			// unknown, do nothing
			break;
	}

	/*
	 * Air Temperature
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->dc.airtemp.mkelvin = C_to_mkelvin((double)(tmp_2bytes / 100));

	/*
	 * Dive suit, values table, 0 to 6
	 */
	read_bytes(1);
	switch (tmp_1byte) {
		case 1:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "No suit"));
			break;
		case 2:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Shorty"));
			break;
		case 3:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Combi"));
			break;
		case 4:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Wet suit"));
			break;
		case 5:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Semidry suit"));
			break;
		case 6:
			dt_dive->suit = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Dry suit"));
			break;
		default:
			// unknown, do nothing
			break;
	}

	/*
	 * Tank, volume size in liter*100. And initialize gasmix to air (default).
	 * Dtrak don't record init and end pressures, but consumed bar, so let's
	 * init a default pressure of 200 bar.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF) {
		dt_dive->cylinder[0].type.size.mliter = tmp_2bytes * 10;
		dt_dive->cylinder[0].type.description = strdup("");
		dt_dive->cylinder[0].start.mbar = 200000;
		dt_dive->cylinder[0].gasmix.he.permille = 0;
		dt_dive->cylinder[0].gasmix.o2.permille = 210;
		dt_dive->cylinder[0].manually_added = true;
	}

	/*
	 * Maximum depth, in cm.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->maxdepth.mm = dt_dive->dc.maxdepth.mm = (int32_t)tmp_2bytes * 10;

	/*
	 * Dive time in minutes.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->duration.seconds = dt_dive->dc.duration.seconds = (uint32_t)tmp_2bytes * 60;

	/*
	 * Minimum water temperature in C*100. If unknown, set it to 0K which
	 * is subsurface's value for "unknown"
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7fff)
		dt_dive->watertemp.mkelvin = dt_dive->dc.watertemp.mkelvin = C_to_mkelvin((double)(tmp_2bytes / 100));
	else
		dt_dive->watertemp.mkelvin = 0;

	/*
	 * Air used in bar*100.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF && dt_dive->cylinder[0].type.size.mliter)
		dt_dive->cylinder[0].gas_used.mliter = dt_dive->cylinder[0].type.size.mliter * (tmp_2bytes / 100.0);

	/*
	 * Dive Type 1 -  Bit table. Subsurface don't have this record, but
	 * will use tags. Bits 0 and 1 are not used. Reuse coincident tags.
	 */
	read_bytes(1);
	byte = byte_to_bits(tmp_1byte);
	if (byte[2] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "no stop")));
	if (byte[3] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "deco")));
	if (byte[4] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "single ascent")));
	if (byte[5] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "multiple ascent")));
	if (byte[6] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "fresh")));
	if (byte[7] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "salt water")));
	free(byte);

	/*
	 * Dive Type 2 - Bit table, use tags again
	 */
	read_bytes(1);
	byte = byte_to_bits(tmp_1byte);
	if (byte[0] != 0) {
		taglist_add_tag(&dt_dive->tag_list, strdup("nitrox"));
		is_nitrox = 1;
	}
	if (byte[1] != 0) {
		taglist_add_tag(&dt_dive->tag_list, strdup("rebreather"));
		is_SCR = 1;
		dt_dive->dc.divemode = PSCR;
	}
	free(byte);

	/*
	 *  Dive Activity 1 - Bit table, use tags again
	 */
	read_bytes(1);
	byte = byte_to_bits(tmp_1byte);
	if (byte[0] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "sight seeing")));
	if (byte[1] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "club dive")));
	if (byte[2] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "instructor")));
	if (byte[3] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "instruction")));
	if (byte[4] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "night")));
	if (byte[5] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "cave")));
	if (byte[6] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "ice")));
	if (byte[7] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "search")));
	free(byte);


	/*
	 * Dive Activity 2 - Bit table, use tags again
	 */
	read_bytes(1);
	byte = byte_to_bits(tmp_1byte);
	if (byte[0] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "wreck")));
	if (byte[1] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "river")));
	if (byte[2] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "drift")));
	if (byte[3] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "photo")));
	if (byte[4] != 0)
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "other")));
	free(byte);

	/*
	 * Other activities - String  1st byte = long
	 * Will put this in dive notes before the true notes
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		snprintf(buffer, sizeof(buffer), "%s: %s\n",
			 QT_TRANSLATE_NOOP("gettextFromC", "Other activities"),
			 tmp_string1);
		tmp_notes_str = strdup(buffer);
		free(tmp_string1);
	}

	/*
	 * Dive buddies
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		dt_dive->buddy = strdup((char *)tmp_string1);
		free(tmp_string1);
	}

	/*
	 * Dive notes
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		int len = snprintf(buffer, sizeof(buffer), "%s%s:\n%s",
				   tmp_notes_str ? tmp_notes_str : "",
				   QT_TRANSLATE_NOOP("gettextFromC", "Datatrak/Wlog notes"),
				   tmp_string1);
		dt_dive->notes = calloc((len +1), 1);
		dt_dive->notes = memcpy(dt_dive->notes, buffer, len);
		free(tmp_string1);
		if (tmp_notes_str != NULL)
			free(tmp_notes_str);
	}

	/*
	 * Alarms 1 - Bit table - Not in Subsurface, we use the profile
	 */
	read_bytes(1);

	/*
	 * Alarms 2 - Bit table - Not in Subsurface, we use the profile
	 */
	read_bytes(1);

	/*
	 * Dive number  (in datatrak, after import user has to renumber)
	 */
	read_bytes(2);
	dt_dive->number = tmp_2bytes;

	/*
	 * Computer timestamp - Useless for Subsurface
	 */
	read_bytes(4);

	/*
	 * Model - table - Not included 0x14, 0x24, 0x41, and 0x73
	 * known to exist, but not its model name - To add in the future.
	 * Strangely 0x00 serves for manually added dives and a dc too, at
	 * least in EXAMPLE.LOG file, shipped with the software.
	 */
	read_bytes(1);
	switch (tmp_1byte) {
		case 0x00:
			dt_dive->dc.model = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Manually entered dive"));
			break;
		case 0x1C:
			dt_dive->dc.model = strdup("Aladin Air");
			break;
		case 0x1D:
			dt_dive->dc.model = strdup("Spiro Monitor 2 plus");
			break;
		case 0x1E:
			dt_dive->dc.model = strdup("Aladin Sport");
			break;
		case 0x1F:
			dt_dive->dc.model = strdup("Aladin Pro");
			break;
		case 0x34:
			dt_dive->dc.model = strdup("Aladin Air X");
			break;
		case 0x3D:
			dt_dive->dc.model = strdup("Spiro Monitor 2 plus");
			break;
		case 0x3F:
			dt_dive->dc.model = strdup("Mares Genius");
			break;
		case 0x44:
			dt_dive->dc.model = strdup("Aladin Air X");
			break;
		case 0x48:
			dt_dive->dc.model = strdup("Spiro Monitor 3 Air");
			break;
		case 0xA4:
			dt_dive->dc.model = strdup("Aladin Air X O2");
			break;
		case 0xB1:
			dt_dive->dc.model = strdup("Citizen Hyper Aqualand");
			break;
		case 0xB2:
			dt_dive->dc.model = strdup("Citizen ProMaster");
			break;
		case 0xB3:
			dt_dive->dc.model = strdup("Mares Guardian");
			break;
		case 0xBC:
			dt_dive->dc.model = strdup("Aladin Air X Nitrox");
			break;
		case 0xF4:
			dt_dive->dc.model = strdup("Aladin Air X Nitrox");
			break;
		case 0xFF:
			dt_dive->dc.model = strdup("Aladin Pro Nitrox");
			break;
		default:
			dt_dive->dc.model = strdup(QT_TRANSLATE_NOOP("gettextFromC", "Unknown"));
			break;
	}
	if ((tmp_1byte & 0xF0) == 0xF0)
		is_nitrox = 1;
	if ((tmp_1byte & 0xF0) == 0xA0)
		is_O2 = 1;

	/*
	 * Air usage, unknown use. Probably allows or deny manually entering gas
	 * comsumption based on dc model - Useless for Subsurface
	 */
	read_bytes(1);
	if (fseek(archivo, 6, 1) != 0)	// jump over 6 bytes whitout known use
		goto bail;
	/*
	 * Profile data length
	 */
	read_bytes(2);
	profile_length = tmp_2bytes;
	if (profile_length != 0) {
		/*
		 * 8 x 2 bytes for the tissues saturation useless for subsurface
		 * and other 6 bytes without known use
		 */
		if (fseek(archivo, 22, 1) != 0)
			goto bail;
		if (is_nitrox || is_O2) {

			/*
			 * CNS  % (unsure) values table (only nitrox computers)
			 */
			read_bytes(1);

			/*
			 * % O2 in nitrox mix - (only nitrox and O2 computers but differents)
			 */
			read_bytes(1);
			if (is_nitrox) {
				dt_dive->cylinder[0].gasmix.o2.permille =
					(tmp_1byte & 0x0F ? 20.0 + 2 * (tmp_1byte & 0x0F) : 21.0) * 10;
			} else {
				dt_dive->cylinder[0].gasmix.o2.permille = tmp_1byte * 10;
				read_bytes(1)  // Jump over one byte, unknown use
			}
		}
		/*
		 * profileLength = Nº bytes, need to know how many samples are there.
		 * 2bytes per sample plus another one each three samples. Also includes the
		 * bytes jumped over (22) and the nitrox (2) or O2 (3).
		 */
		int samplenum = is_O2 ? (profile_length - 25) * 3 / 8 : (profile_length - 24) * 3 / 7;

		dc->events = calloc(samplenum, sizeof(struct event));
		dc->alloc_samples = samplenum;
		dc->samples = 0;
		dc->sample = calloc(samplenum, sizeof(struct sample));

		dtrak_profile(dt_dive, archivo);
	}
	/*
	 * Initialize some dive data not supported by Datatrak/WLog
	 */
	if (!strcmp(dt_dive->dc.model, "Manually entered dive"))
		dt_dive->dc.deviceid = 0;
	else
		dt_dive->dc.deviceid = 0xffffffff;
	create_device_node(dt_dive->dc.model, dt_dive->dc.deviceid, "", "", dt_dive->dc.model);
	dt_dive->dc.next = NULL;
	if (!is_SCR && dt_dive->cylinder[0].type.size.mliter) {
		dt_dive->cylinder[0].end.mbar = dt_dive->cylinder[0].start.mbar -
			((dt_dive->cylinder[0].gas_used.mliter / dt_dive->cylinder[0].type.size.mliter) * 1000);
	}
	return true;

bail:
	return false;
}

void datatrak_import(const char *file, struct dive_table *table)
{
	FILE *archivo;
	dtrakheader *fileheader = (dtrakheader *)malloc(sizeof(dtrakheader));
	int i = 0;

	if ((archivo = subsurface_fopen(file, "rb")) == NULL) {
		report_error(translate("gettextFromC", "Error: couldn't open the file %s"), file);
		free(fileheader);
		return;
	}

	/*
	 * Verify fileheader,  get number of dives in datatrak divelog
	 */
	*fileheader = read_file_header(archivo);
	while (i < fileheader->divesNum) {
		struct dive *ptdive = alloc_dive();

		if (!dt_dive_parser(archivo, ptdive)) {
			report_error(translate("gettextFromC", "Error: no dive"));
			free(ptdive);
		} else {
			record_dive(ptdive);
		}
		i++;
	}
	taglist_cleanup(&g_tag_list);
	fclose(archivo);
	sort_table(table);
	free(fileheader);
}
