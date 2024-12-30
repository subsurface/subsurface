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
#include "format.h"
#include "divesite.h"
#include "dive.h"
#include "divelog.h"
#include "errorhelper.h"
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

	char *out_string = (char *)calloc(outlen, 1);
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
static int dtrak_prepare_data(int model, device_data_t &dev_data)
{
	dc_descriptor_t *d = NULL;
	int i = 0;

	while (model != g_models[i].model_num && g_models[i].model_num != 0xEE)
		i++;
	dev_data.model = g_models[i].name;
	dev_data.vendor.clear();
	for (const char *s = g_models[i].name; isalpha(*s); ++s)
		dev_data.vendor += *s;
	dev_data.product = strchr(g_models[i].name, ' ') + 1;

	d = get_descriptor(g_models[i].type, g_models[i].libdc_num);
	if (d)
		dev_data.descriptor = d;
	else
		return 0;
	return g_models[i].libdc_num;
}

/*
 * Return a default name for a tank based on it's size.
 * Just get the first in the user's list for given size.
 * Reaching the end of the list means there is no tank of this size.
 */
static std::string cyl_type_by_size(int size)
{
	auto it = std::find_if(tank_info_table.begin(), tank_info_table.end(),
			       [size] (const tank_info &info) { return info.ml == size; });
	return it != tank_info_table.end() ? it->name : std::string();
}

/*
 * Reads the size of a datatrak profile from actual position in buffer *ptr,
 * zero padds it with a faked header and inserts the model number for
 * libdivecomputer parsing. Puts the completed buffer in a pre-allocated
 * compl_buffer, and returns status.
 */
dc_status_t dt_libdc_buffer(unsigned char *ptr, int prf_length, int dc_model, unsigned char *compl_buffer)
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
static char *dt_dive_parser(unsigned char *runner, struct dive *dt_dive, struct divelog *log, char *maxbuf)
{
	int  rc, profile_length, libdc_model;
	unsigned char *tmp_string1 = NULL,
		      *locality = NULL,
		      *dive_point = NULL,
		      *compl_buffer,
		      *membuf = runner;
	unsigned char tmp_1byte;
	unsigned int tmp_2bytes;
	unsigned long tmp_4bytes;
	std::string tmp_notes_str;
	char is_nitrox = 0, is_O2 = 0, is_SCR = 0;

	device_data_t devdata;
	devdata.log = log;

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
	dt_dive->dcs[0].when = dt_dive->when = (timestamp_t)date_time_to_ssrfc(tmp_4bytes, tmp_2bytes);

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
	{
		std::string buffer2 = std::string((char *)locality) + " " + (char *)dive_point;
		struct dive_site *ds = log->sites.get_by_name(buffer2);
		if (!ds)
			ds = log->sites.create(buffer2);
		ds->add_dive(dt_dive);
	}
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
			dt_dive->dcs[0].surface_pressure = 1_atm;
			break;
		case 2:
			dt_dive->dcs[0].surface_pressure = 932_mbar;
			break;
		case 3:
			dt_dive->dcs[0].surface_pressure = 828_mbar;
			break;
		case 4:
			dt_dive->dcs[0].surface_pressure = 735_mbar;
			break;
		default:
			dt_dive->dcs[0].surface_pressure = 1_atm;
	}

	/*
	 * Interval (minutes)
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->dcs[0].surfacetime.seconds = (uint32_t) tmp_2bytes * 60;

	/*
	 * Weather, values table, 0 to 6
	 * Subsurface don't have this record but we can use tags
	 */
	dt_dive->tags.clear();
	read_bytes(1);
	switch (tmp_1byte) {
		case 1:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "clear"));
			break;
		case 2:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "misty"));
			break;
		case 3:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "fog"));
			break;
		case 4:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "rain"));
			break;
		case 5:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "storm"));
			break;
		case 6:
			taglist_add_tag(dt_dive->tags, translate("gettextFromC", "snow"));
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
		dt_dive->dcs[0].airtemp.mkelvin = C_to_mkelvin((double)(tmp_2bytes / 100));

	/*
	 * Dive suit, values table, 0 to 6
	 */
	read_bytes(1);
	switch (tmp_1byte) {
		case 1:
			dt_dive->suit = "No suit";
			break;
		case 2:
			dt_dive->suit = "Shorty";
			break;
		case 3:
			dt_dive->suit = "Combi";
			break;
		case 4:
			dt_dive->suit = "Wet suit";
			break;
		case 5:
			dt_dive->suit = "Semidry suit";
			break;
		case 6:
			dt_dive->suit = "Dry suit";
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
		cylinder_t cyl;
		cyl.type.size.mliter = tmp_2bytes * 10;
		cyl.type.description = cyl_type_by_size(tmp_2bytes * 10);
		cyl.start = 200_bar;
		cyl.gasmix.he = 0_percent;
		cyl.gasmix.o2 = 21_percent;
		cyl.manually_added = true;
		dt_dive->cylinders.push_back(std::move(cyl));
	}

	/*
	 * Maximum depth, in cm.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->maxdepth.mm = dt_dive->dcs[0].maxdepth.mm = (int32_t)tmp_2bytes * 10;

	/*
	 * Dive time in minutes.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF)
		dt_dive->duration.seconds = dt_dive->dcs[0].duration.seconds = (uint32_t)tmp_2bytes * 60;

	/*
	 * Minimum water temperature in C*100. If unknown, set it to 0K which
	 * is subsurface's value for "unknown"
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7fff)
		dt_dive->watertemp.mkelvin = dt_dive->dcs[0].watertemp.mkelvin = C_to_mkelvin((double)(tmp_2bytes / 100));
	else
		dt_dive->watertemp = 0_K;

	/*
	 * Air used in bar*100.
	 */
	read_bytes(2);
	if (tmp_2bytes != 0x7FFF && dt_dive->cylinders.size() > 0)
		dt_dive->get_cylinder(0)->gas_used.mliter = lrint(dt_dive->get_cylinder(0)->type.size.mliter * (tmp_2bytes / 100.0));

	/*
	 * Dive Type 1 -  Bit table. Subsurface don't have this record, but
	 * will use tags. Bits 0 and 1 are not used. Reuse coincident tags.
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "no stop"));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "deco"));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "single ascent"));
	if (bit_set(tmp_1byte, 5))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "multiple ascent"));
	if (bit_set(tmp_1byte, 6))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "fresh water"));
	if (bit_set(tmp_1byte, 7))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "salt water"));

	/*
	 * Dive Type 2 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0)) {
		taglist_add_tag(dt_dive->tags, "nitrox");
		is_nitrox = 1;
	}
	if (bit_set(tmp_1byte, 1)) {
		taglist_add_tag(dt_dive->tags, "rebreather");
		is_SCR = 1;
		dt_dive->dcs[0].divemode = PSCR;
	}

	/*
	 *  Dive Activity 1 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "sight seeing"));
	if (bit_set(tmp_1byte, 1))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "club dive"));
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "instructor"));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "instruction"));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "night"));
	if (bit_set(tmp_1byte, 5))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "cave"));
	if (bit_set(tmp_1byte, 6))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "ice"));
	if (bit_set(tmp_1byte, 7))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "search"));

	/*
	 * Dive Activity 2 - Bit table, use tags again
	 */
	read_bytes(1);
	if (bit_set(tmp_1byte, 0))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "wreck"));
	if (bit_set(tmp_1byte, 1))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "river"));
	if (bit_set(tmp_1byte, 2))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "drift"));
	if (bit_set(tmp_1byte, 3))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "photo"));
	if (bit_set(tmp_1byte, 4))
		taglist_add_tag(dt_dive->tags, translate("gettextFromC", "other"));

	/*
	 * Other activities - String  1st byte = long
	 * Will put this in dive notes before the true notes
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		tmp_notes_str= format_string_std("%s: %s\n",
			 translate("gettextFromC", "Other activities"),
			 tmp_string1);
		free(tmp_string1);
	}

	/*
	 * Dive buddies
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		dt_dive->buddy = (const char *)tmp_string1;
		free(tmp_string1);
	}

	/*
	 * Dive notes
	 */
	read_bytes(1);
	if (tmp_1byte != 0) {
		read_string(tmp_string1);
		dt_dive->notes = format_string_std("%s%s:\n%s",
				   tmp_notes_str.c_str(),
				   translate("gettextFromC", "Datatrak/Wlog notes"),
				   tmp_string1);
		free(tmp_string1);
	}

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
	dt_dive->dcs[0].model = devdata.model;

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
			libdc_buffer_parser(dt_dive, &devdata, compl_buffer, profile_length + 18);
		} else {
			report_error(translate("gettextFromC", "[Error] Out of memory for dive %d. Abort parsing."), dt_dive->number);
			free(compl_buffer);
			goto bail;
		}
		if (is_nitrox && dt_dive->cylinders.size() > 0)
			dt_dive->get_cylinder(0)->gasmix.o2.permille =
					lrint(membuf[23] & 0x0F ? 20.0 + 2 * (membuf[23] & 0x0F) : 21.0) * 10;
		if (is_O2 && dt_dive->cylinders.size() > 0)
			dt_dive->get_cylinder(0)->gasmix.o2.permille = membuf[23] * 10;
		free(compl_buffer);
	}
	JUMP(membuf, profile_length);

	/*
	 * Initialize some dive data not supported by Datatrak/WLog
	 */
	if (!libdc_model)
		dt_dive->dcs[0].deviceid = 0;
	else
		dt_dive->dcs[0].deviceid = 0xffffffff;
	if (!is_SCR && dt_dive->cylinders.size() > 0) {
		dt_dive->get_cylinder(0)->end.mbar = dt_dive->get_cylinder(0)->start.mbar -
			((dt_dive->get_cylinder(0)->gas_used.mliter / dt_dive->get_cylinder(0)->type.size.mliter) * 1000);
	}
	return (char *)membuf;
bail:
	free(locality);
	return NULL;
}

/*
 * Parses the header of the .add file, returns the number of dives in
 * the archive (must be the same than number of dives in .log file).
 */
static int wlog_header_parser (std::string &mem)
{
	int tmp;
	unsigned char *runner = (unsigned char *) mem.data();
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
static void wlog_compl_parser(std::string &wl_mem, struct dive *dt_dive, int dcount)
{
	int tmp = 0, offset = 12 + (dcount * 850),
	    pos_weight =  offset + 256,
	    pos_viz = offset + 258,
	    pos_tank_init = offset + 266,
	    pos_suit = offset + 268;
	char *wlog_notes = NULL, *wlog_suit = NULL;
	unsigned char *runner = (unsigned char *) wl_mem.data();

	/*
	 * Extended notes string. Fixed length 256 bytes. 0 padded if not complete
	 */
	if (*(runner + offset)) {
		char wlog_notes_temp[NOTES_LENGTH + 1];
		wlog_notes_temp[NOTES_LENGTH] = 0;
		(void)memcpy(wlog_notes_temp, runner + offset, NOTES_LENGTH);
		wlog_notes = to_utf8((unsigned char *) wlog_notes_temp);
	}
	if (wlog_notes)
		dt_dive->notes += wlog_notes;

	/*
	 * Weight in Kg * 100
	 */
	tmp = (int) two_bytes_to_int(runner[pos_weight + 1], runner[pos_weight]);
	if (tmp != 0x7fff) {
		weightsystem_t ws = { {.grams = tmp * 10}, translate("gettextFromC", "unknown"), false };
		dt_dive->weightsystems.push_back(std::move(ws));
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
		dt_dive->get_cylinder(0)->start.mbar = tmp * 10;
		dt_dive->get_cylinder(0)->end.mbar =  dt_dive->get_cylinder(0)->start.mbar - lrint(dt_dive->get_cylinder(0)->gas_used.mliter / dt_dive->get_cylinder(0)->type.size.mliter) * 1000;
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
		dt_dive->suit = wlog_suit;
	free(wlog_suit);
}

/*
 * Main function call from file.c data is allocated (and freed) there.
 * If parsing is aborted due to errors, stores correctly parsed dives.
 */
int datatrak_import(std::string &mem, std::string &wl_mem, struct divelog *log)
{
	char *runner;
	int i = 0, numdives = 0, rc = 0;

	char *maxbuf = mem.data() + mem.size();

	// Verify fileheader,  get number of dives in datatrak divelog, zero on error
	numdives = read_file_header((unsigned char *)mem.data());
	if (!numdives) {
		report_error("%s", translate("gettextFromC", "[Error] File is not a DataTrak file. Aborted"));
		goto bail;
	}
	// Verify WLog .add file, Beginning sequence and Nº dives
	if(!wl_mem.empty()) {
		int compl_dives_n = wlog_header_parser(wl_mem);
		if (compl_dives_n != numdives) {
			report_error("ERROR: Not the same number of dives in .log %d and .add file %d.\nWill not parse .add file", numdives , compl_dives_n);
			wl_mem.clear();
		}
	}
	// Point to the expected begining of 1st. dive data
	runner = mem.data();
	JUMP(runner, 12);

	// Sequential parsing. Abort if received NULL from dt_dive_parser.
	while ((i < numdives) && (runner < maxbuf)) {
		auto ptdive = std::make_unique<dive>();

		runner = dt_dive_parser((unsigned char *)runner, ptdive.get(), log, maxbuf);
		if (!wl_mem.empty())
			wlog_compl_parser(wl_mem, ptdive.get(), i);
		if (runner == NULL) {
			report_error("%s", translate("gettextFromC", "Error: no dive"));
			rc = 1;
			goto out;
		} else {
			log->dives.record_dive(std::move(ptdive));
		}
		i++;
	}
out:
	log->dives.sort();
	return rc;
bail:
	return 1;
}
