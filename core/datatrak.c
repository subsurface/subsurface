// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "gettext.h"
#include "datatrak.h"
#include "subsurface-string.h"
#include "units.h"
#include "device.h"
#include "file.h"
#include "divesite.h"
#include "dive.h"
#include "errorhelper.h"
#include "ssrf.h"
#include "tag.h"

static unsigned int two_bytes_to_int(unsigned char x, unsigned char y)
{
	return (x << 8) + y;
}

static unsigned long four_bytes_to_long(unsigned char x, unsigned char y, unsigned char z, unsigned char t)
{
	return ((long)x << 24) + ((long)y << 16) + ((long)z << 8) + (long)t;
}

static bool bit_set(unsigned char byte, int bit)
{
	return byte & (1 << bit);
}

/*
 * Datatrak stores the date in days since 01-01-1600, while Subsurface uses
 * time_t (seconds since 00:00 01-01-1970). Function subtracts
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
		if (in_string[i] < 127) {
			out_string[j] = in_string[i];
		} else {
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
 * Reads the header of a datatrak  buffer and returns the number of
 * dives; zero on error (meaning this isn't a datatrak file).
 * All other info in the header is useless for Subsurface.
 */
static int read_file_header(unsigned char *buffer)
{
	int n = 0;

	if (two_bytes_to_int(buffer[0], buffer[1]) == 0xA100)
		n = two_bytes_to_int(buffer[7], buffer[6]);
	return n;
}

/*
 * Fills a device_data_t structure based on the info from g_models table, using
 * the dc's model number as start point.
 * Returns libdc's equivalent model number (also from g_models) or zero if
 * this a manual dive.
 */
static int dtrak_prepare_data(int model, device_data_t *dev_data)
{
	dc_descriptor_t *d = NULL;
	int i = 0;

	while (model != g_models[i].model_num && g_models[i].model_num != 0xEE)
		i++;
	dev_data->model = copy_string(g_models[i].name);
	dev_data->vendor = (const char *)malloc(strlen(g_models[i].name) + 1);
	sscanf(g_models[i].name, "%[A-Za-z] ", (char *)dev_data->vendor);
	dev_data->product = copy_string(strchr(g_models[i].name, ' ') + 1);

	d = get_descriptor(g_models[i].type, g_models[i].libdc_num);
	if (d)
		dev_data->descriptor = d;
	else
		return 0;
	return g_models[i].libdc_num;
}

/*
 * Return a default name for a tank based on it's size.
 * Just get the first in the user's list for given size.
 * Reaching the end of the list means there is no tank of this size.
 */
static const char *cyl_type_by_size(int size)
{
	for (int i = 0; i < tank_info_table.nr; ++i) {
		const struct tank_info *ti = &tank_info_table.infos[i];
		if (ti->ml == size)
			return ti->name;
	}
	return "";
}

/*
 * Reads the size of a datatrak profile from actual position in buffer *ptr,
 * zero padds it with a faked header and inserts the model number for
 * libdivecomputer parsing. Puts the completed buffer in a pre-allocated
 * compl_buffer, and returns status.
 */
static dc_status_t dt_libdc_buffer(unsigned char *ptr, int prf_length, int dc_model, unsigned char *compl_buffer)
{
	if (compl_buffer == NULL)
		return DC_STATUS_NOMEMORY;
	compl_buffer[3] = (unsigned char) dc_model;
	memcpy(compl_buffer + 18, ptr, prf_length);
	return DC_STATUS_SUCCESS;
}

/*
 * Parses a mem buffer extracting its data and filling a subsurface's dive structure.
 * Returns a pointer to last position in buffer, or NULL on failure.
 */
static unsigned char *dt_dive_parser(unsigned char *runner, struct dive *dt_dive, struct dive_site_table *sites,
				     struct device_table *devices, long maxbuf)
{
	int  rc, profile_length, libdc_model;
	char *tmp_notes_str = NULL;
	unsigned char *tmp_string1 = NULL,
		      *locality = NULL,
		      *dive_point = NULL,
		      *compl_buffer,
		      *membuf = runner;
	char buffer[1024];
	unsigned char tmp_1byte;
	unsigned int tmp_2bytes;
	unsigned long tmp_4bytes;
	struct dive_site *ds;
	char is_nitrox = 0, is_O2 = 0, is_SCR = 0;

	device_data_t *devdata = calloc(1, sizeof(device_data_t));
	devdata->sites = sites;
	devdata->devices = devices;

	/*
	 * Parse byte to byte till next dive entry
	 */
	while (membuf[0] != 0xA0 || membuf[1] != 0x00) {
		JUMP(membuf, 1);
	}
	JUMP(membuf, 2);

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
	ds = get_dive_site_by_name(buffer, sites);
	if (!ds)
		ds = create_dive_site(buffer, sites);
	add_dive_to_dive_site(dt_dive, ds);
	free(locality);
	locality = NULL;
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
	 * Dtrak doesn't record init and end pressures, but consumed bar, so let's
	 * init a default pressure of 200 bar.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF) {
		cylinder_t cyl = empty_cylinder;
		cyl.type.size.mliter = tmp_2bytes * 10;
		cyl.type.description = cyl_type_by_size(tmp_2bytes * 10);
		cyl.start.mbar = 200000;
		cyl.gasmix.he.permille = 0;
		cyl.gasmix.o2.permille = 210;
		cyl.manually_added = true;
		add_cloned_cylinder(&dt_dive->cylinders, cyl);
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
	if (tmp_2bytes != 0x7FFF && dt_dive->cylinders.nr > 0)
		get_cylinder(dt_dive, 0)->gas_used.mliter = lrint(get_cylinder(dt_dive, 0)->type.size.mliter * (tmp_2bytes / 100.0));

	/*
	 * Dive Type 1 -  Bit table. Subsurface don't have this record, but
	 * will use tags. Bits 0 and 1 are not used. Reuse coincident tags.
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "no stop")));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "deco")));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "single ascent")));
	if (bit_set(tmp_1byte, 5))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "multiple ascent")));
	if (bit_set(tmp_1byte, 6))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "fresh water")));
	if (bit_set(tmp_1byte, 7))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "salt water")));

	/*
	 * Dive Type 2 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0)) {
		taglist_add_tag(&dt_dive->tag_list, strdup("nitrox"));
		is_nitrox = 1;
	}
	if (bit_set(tmp_1byte, 1)) {
		taglist_add_tag(&dt_dive->tag_list, strdup("rebreather"));
		is_SCR = 1;
		dt_dive->dc.divemode = PSCR;
	}

	/*
	 *  Dive Activity 1 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "sight seeing")));
	if (bit_set(tmp_1byte, 1))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "club dive")));
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "instructor")));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "instruction")));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "night")));
	if (bit_set(tmp_1byte, 5))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "cave")));
	if (bit_set(tmp_1byte, 6))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "ice")));
	if (bit_set(tmp_1byte, 7))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "search")));

	/*
	 * Dive Activity 2 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "wreck")));
	if (bit_set(tmp_1byte, 1))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "river")));
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "drift")));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "photo")));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(&dt_dive->tag_list, strdup(QT_TRANSLATE_NOOP("gettextFromC", "other")));

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
	}
	free(tmp_notes_str);

	/*
	 * Alarms 1 and Alarms2 - Bit tables - Not in Subsurface, we use the profile
	 */
	JUMP(membuf, 2);

	/*
	 * Dive number  (in datatrak, after import user has to renumber)
	 */
	read_bytes(2);
	dt_dive->number = tmp_2bytes;

	/*
	 * Computer timestamp - Useless for Subsurface
	 */
	JUMP(membuf, 4);

	/*
	 * Model number to check against equivalence with libdivecomputer table.
	 * The number also defines if the model is nitrox or O2 capable.
	 */
	read_bytes(1);
	switch (tmp_1byte & 0xF0) {
		case 0xF0:
			is_nitrox = 1;
			break;
		case 0xA0:
			is_O2 = 1;
			break;
		default:
			is_nitrox = 0;
			is_O2 = 0;
			break;
	}
	libdc_model = dtrak_prepare_data(tmp_1byte, devdata);
	if (!libdc_model)
		report_error(translate("gettextFromC", "[Warning] Manual dive # %d\n"), dt_dive->number);
	dt_dive->dc.model = copy_string(devdata->model);

	/*
	 * Air usage, unknown use. Probably allows or deny manually entering gas
	 * comsumption based on dc model - Useless for Subsurface
	 * And 6 bytes without known use.
	 */
	JUMP(membuf, 7);

	/*
	 * Profile data length
	 */
	read_bytes(2);
	profile_length = tmp_2bytes;

	/*
	 * Profile parsing, only if we have a profile and a dc model.
	 * If just a profile, skip parsing and seek the buffer to the end of dive.
	 */
	if (profile_length != 0 && libdc_model != 0) {
		compl_buffer = (unsigned char *) calloc(18 + profile_length, 1);
		rc = dt_libdc_buffer(membuf, profile_length, libdc_model, compl_buffer);
		if (rc == DC_STATUS_SUCCESS) {
			libdc_buffer_parser(dt_dive, devdata, compl_buffer, profile_length + 18);
		} else {
			report_error(translate("gettextFromC", "[Error] Out of memory for dive %d. Abort parsing."), dt_dive->number);
			free(compl_buffer);
			goto bail;
		}
		if (is_nitrox && dt_dive->cylinders.nr > 0)
			get_cylinder(dt_dive, 0)->gasmix.o2.permille =
					lrint(membuf[23] & 0x0F ? 20.0 + 2 * (membuf[23] & 0x0F) : 21.0) * 10;
		if (is_O2 && dt_dive->cylinders.nr > 0)
			get_cylinder(dt_dive, 0)->gasmix.o2.permille = membuf[23] * 10;
		free(compl_buffer);
	}
	JUMP(membuf, profile_length);

	/*
	 * Initialize some dive data not supported by Datatrak/WLog
	 */
	if (!libdc_model)
		dt_dive->dc.deviceid = 0;
	else
		dt_dive->dc.deviceid = 0xffffffff;
	create_device_node(devices, dt_dive->dc.model, dt_dive->dc.deviceid, "", "", dt_dive->dc.model);
	dt_dive->dc.next = NULL;
	if (!is_SCR && dt_dive->cylinders.nr > 0) {
		get_cylinder(dt_dive, 0)->end.mbar = get_cylinder(dt_dive, 0)->start.mbar -
			((get_cylinder(dt_dive, 0)->gas_used.mliter / get_cylinder(dt_dive, 0)->type.size.mliter) * 1000);
	}
	free(devdata);
	return membuf;
bail:
	free(locality);
	free(devdata);
	return NULL;
}

/*
 * Parses the header of the .add file, returns the number of dives in
 * the archive (must be the same than number of dives in .log file).
 */
static int wlog_header_parser (struct memblock *mem)
{
	int tmp;
	unsigned char *runner = (unsigned char *) mem->buffer;
	if (!runner)
		return -1;
	if (!memcmp(runner, "\x52\x02", 2)) {
		runner += 8;
		tmp = (int) two_bytes_to_int(runner[1], runner[0]);
		return tmp;
	} else {
		fprintf(stderr, "Error, not a Wlog .add file\n");
		return -1;
	}
}

#define NOTES_LENGTH 256
#define SUIT_LENGTH 26
static void wlog_compl_parser(struct memblock *wl_mem, struct dive *dt_dive, int dcount)
{
	int tmp = 0, offset = 12 + (dcount * 850),
	    pos_weight =  offset + 256,
	    pos_viz = offset + 258,
	    pos_tank_init = offset + 266,
	    pos_suit = offset + 268;
	char *wlog_notes = NULL, *wlog_suit = NULL, *buffer = NULL;
	unsigned char *runner = (unsigned char *) wl_mem->buffer;

	/*
	 * Extended notes string. Fixed length 256 bytes. 0 padded if not complete
	 */
	if (*(runner + offset)) {
		char wlog_notes_temp[NOTES_LENGTH + 1];
		wlog_notes_temp[NOTES_LENGTH] = 0;
		(void)memcpy(wlog_notes_temp, runner + offset, NOTES_LENGTH);
		wlog_notes = to_utf8((unsigned char *) wlog_notes_temp);
	}
	if (dt_dive->notes && wlog_notes) {
		buffer = calloc (strlen(dt_dive->notes) + strlen(wlog_notes) + 1, 1);
		sprintf(buffer, "%s%s", dt_dive->notes, wlog_notes);
		free(dt_dive->notes);
		dt_dive->notes = copy_string(buffer);
	} else if (wlog_notes) {
		dt_dive->notes = copy_string(wlog_notes);
	}
	free(buffer);
	free(wlog_notes);

	/*
	 * Weight in Kg * 100
	 */
	tmp = (int) two_bytes_to_int(runner[pos_weight + 1], runner[pos_weight]);
	if (tmp != 0x7fff) {
		weightsystem_t ws = { {lrint(tmp * 10)}, QT_TRANSLATE_NOOP("gettextFromC", "unknown") };
		add_cloned_weightsystem(&dt_dive->weightsystems, ws);
	}

	/*
	 * Visibility in m * 100.  Arbitrarily choosed to be 5 stars if >= 25m and
	 * then assign a star for each 5 meters, resulting 0 stars if < 5 m
	 */
	tmp = (int) two_bytes_to_int(runner[pos_viz + 1], runner[pos_viz]);
	if (tmp != 0x7fff) {
		tmp = tmp > 2500 ? 2500 / 100 : tmp / 100;
		dt_dive->visibility = (int) floor(tmp / 5);
	}

	/*
	 * Tank initial pressure in bar * 100
	 * If we know initial pressure, rework end pressure.
	 */
	tmp = (int) two_bytes_to_int(runner[pos_tank_init + 1], runner[pos_tank_init]);
	if (tmp != 0x7fff) {
		get_cylinder(dt_dive, 0)->start.mbar = tmp * 10;
		get_cylinder(dt_dive, 0)->end.mbar =  get_cylinder(dt_dive, 0)->start.mbar - lrint(get_cylinder(dt_dive, 0)->gas_used.mliter / get_cylinder(dt_dive, 0)->type.size.mliter) * 1000;
	}

	/*
	 * Dive suit, fixed length of 26 bytes, zero padded if shorter.
	 * Expected to be preferred by the user if he did the work of setting it.
	 */
	if (*(runner + pos_suit)) {
		char wlog_suit_temp[SUIT_LENGTH + 1];
		wlog_suit_temp[SUIT_LENGTH] = 0;
		(void)memcpy(wlog_suit_temp, runner + pos_suit, SUIT_LENGTH);
		wlog_suit = to_utf8((unsigned char *) wlog_suit_temp);
	}
	if (wlog_suit)
		dt_dive->suit = copy_string(wlog_suit);
	free(wlog_suit);
}

/*
 * Main function call from file.c memblock is allocated (and freed) there.
 * If parsing is aborted due to errors, stores correctly parsed dives.
 */
int datatrak_import(struct memblock *mem, struct memblock *wl_mem, struct dive_table *table, struct trip_table *trips,
		    struct dive_site_table *sites, struct device_table *devices)
{
	UNUSED(trips);
	unsigned char *runner;
	int i = 0, numdives = 0, rc = 0;

	long maxbuf = (long) mem->buffer + mem->size;

	// Verify fileheader,  get number of dives in datatrak divelog, zero on error
	numdives = read_file_header((unsigned char *)mem->buffer);
	if (!numdives) {
		report_error(translate("gettextFromC", "[Error] File is not a DataTrak file. Aborted"));
		goto bail;
	}
	// Verify WLog .add file, Beginning sequence and Nº dives
	if(wl_mem) {
		int compl_dives_n = wlog_header_parser(wl_mem);
		if (compl_dives_n != numdives) {
			report_error("ERROR: Not the same number of dives in .log %d and .add file %d.\nWill not parse .add file", numdives , compl_dives_n);
			free(wl_mem->buffer);
			wl_mem->buffer = NULL;
			wl_mem = NULL;
		}
	}
	// Point to the expected begining of 1st. dive data
	runner = (unsigned char *)mem->buffer;
	JUMP(runner, 12);

	// Secuential parsing. Abort if received NULL from dt_dive_parser.
	while ((i < numdives) && ((long) runner < maxbuf)) {
		struct dive *ptdive = alloc_dive();

		runner = dt_dive_parser(runner, ptdive, sites, devices, maxbuf);
		if (wl_mem)
			wlog_compl_parser(wl_mem, ptdive, i);
		if (runner == NULL) {
			report_error(translate("gettextFromC", "Error: no dive"));
			free(ptdive);
			rc = 1;
			goto out;
		} else {
			record_dive_to_table(ptdive, table);
		}
		i++;
	}
out:
	taglist_cleanup(&g_tag_list);
	sort_dive_table(table);
	return rc;
bail:
	return 1;
}
