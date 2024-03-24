// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdlib.h>
#include "qthelper.h"
#include "ssrf.h"
#include "dive.h"
#include "sample.h"
#include "subsurface-string.h"
#include "parse.h"
#include "divelist.h"
#include "divelog.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"
#include "tag.h"
#include "errorhelper.h"
#include <string.h>
#include "divecomputer.h"

/* Process gas change event for seac database.
 * Create gas change event at the time of the
 * current sample.
 */
static int seac_gaschange(void *param, sqlite3_stmt *sqlstmt)
{
	struct parser_state *state = (struct parser_state *)param;

	event_start(state);
	state->cur_event.time.seconds = sqlite3_column_int(sqlstmt, 1);
	strcpy(state->cur_event.name, "gaschange");
	state->cur_event.gas.mix.o2.permille = 10 * sqlite3_column_int(sqlstmt, 4);
	event_end(state);

	return 0;
}

/* Callback function to parse seac dives. Reads headers_dive table to read dive
 * information into divecomputer struct.
 */
static int seac_dive(void *param, int, char **data, char **)
{
	int retval = 0, cylnum = 0;
	int year, month, day, hour, min, sec, tz;
	char isodatetime[30];
	time_t divetime;
	struct gasmix lastgas, curgas;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	sqlite3_stmt *sqlstmt;

	const char *get_samples = "SELECT dive_number, runtime_s, depth_cm, temperature_mCx10, active_O2_fr, first_stop_depth_cm, first_stop_time_s, ndl_tts_s, cns, gf_l, gf_h FROM dive_data WHERE dive_number = ? AND dive_id = ? ORDER BY runtime_s ASC";
		/*  0 = dive_number
		 *  1 = runtime_s
		 *  2 = depth_cm
		 *  3 = temperature_mCx10  - eg dC
		 *  4 = active_O2_fr
		 *  5 = first_stop_depth_cm
		 *  6 = first_stop_time_s
		 *  7 = ndl_tts_s
		 *  8 = cns
		 *  9 = gf-l
		 * 10 = gf-h
		 */

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	// Create first cylinder
	cylinder_t *curcyl = get_or_create_cylinder(state->cur_dive, 0);

	// Get time and date
	sscanf(data[2], "%d/%d/%2d", &day, &month, &year);
	sscanf(data[4], "%2d:%2d:%2d", &hour, &min, &sec);
	year += 2000;

	tz = atoi(data[3]);
	// Timezone offset lookup array
	const char timezoneoffset[][7] =
	{"+12:00",  //  0
	 "+11:00",  //  1
	 "+10:00",  //  2
	 "+09:30",  //  3
	 "+09:00",  //  4
	 "+08:00",  //  5
	 "+07:00",  //  6
	 "+06:00",  //  7
	 "+05:00",  //  8
	 "+04:30",  //  9
	 "+04:00",  // 10
	 "+03:30",  // 11
	 "+03:00",  // 12
	 "+02:00",  // 13
	 "+01:00",  // 14
	 "+00:00",  // 15
	 "-01:00",  // 16
	 "-02:00",  // 17
	 "-03:00",  // 18
	 "-03:30",  // 19
	 "-04:00",  // 20
	 "-04:30",  // 21
	 "-05:00",  // 22
	 "-05:30",  // 23
	 "-05:45",  // 24
	 "-06:00",  // 25
	 "-06:30",  // 26
	 "-07:00",  // 27
	 "-08:00",  // 28
	 "-08:45",  // 29
	 "-09:00",  // 30
	 "-09:30",  // 31
	 "-09:45",  // 32
	 "-10:00",  // 33
	 "-10:30",  // 34
	 "-11:00",  // 35
	 "-11:30",  // 36
	 "-12:00",  // 37
	 "-12:45",  // 38
	 "-13:00",  // 39
	 "-13:45",  // 40
	 "-14:00"}; // 41

	sprintf(isodatetime, "%4i-%02i-%02iT%02i:%02i:%02i%6s", year, month, day, hour, min, sec, timezoneoffset[tz]);
	divetime = get_dive_datetime_from_isostring(isodatetime);
	state->cur_dive->when = divetime;

	// 6 = dive_type
	// Dive type 2?
	if (data[6]) {
		switch (atoi(data[6])) {
		case 1:
			state->cur_dive->dc.divemode = OC;
			break;
		// Gauge Mode
		case 2:
			state->cur_dive->dc.divemode = UNDEF_COMP_TYPE;
			break;
		case 3:
			state->cur_dive->dc.divemode = FREEDIVE;
			break;
		default:
			if (verbose) {
				report_info("Unknown divetype %i", atoi(data[6]));
			}
		}
	}

	// 9 = comments from seac app
	if (data[9]) {
		utf8_string(data[9], &state->cur_dive->notes);
	}

	// 10 = dive duration
	if (data[10]) {
		state->cur_dive->dc.duration.seconds = atoi(data[10]);
	}

	// 8 = water_type
	/* TODO: Seac only offers fresh / salt, and doesn't
	 * seem to record correctly currently. I have both
	 * fresh and saltwater dives and water type is reported
	 * as 150 for both.
	 */
	if (data[8]) {
		switch (atoi(data[8])) {
		case 150:
			state->cur_dive->salinity = 0;
			break;
		case 100:
			state->cur_dive->salinity = 1;
			break;
		default:
			if (verbose) {
				report_info("Unknown salinity %i", atoi(data[8]));
			}
		}
	}


	if (data[11]) {
		state->cur_dive->dc.maxdepth.mm = 10 * atoi(data[11]);
	}

	// Create sql_stmt type to query DB
	retval = sqlite3_prepare_v2(handle, get_samples, -1, &sqlstmt, 0);
	if (retval != SQLITE_OK) {
		report_info("Preparing SQL object failed when getting SeacSync dives.");
		return 1;
	}

	// Bind current dive number to sql statement
	sqlite3_bind_int(sqlstmt, 1, state->cur_dive->number);
	sqlite3_bind_int(sqlstmt, 2, atoi(data[13]));

	// Catch a bad query
	retval = sqlite3_step(sqlstmt);
	if (retval == SQLITE_ERROR) {
		report_info("Getting dive data from SeacSync DB failed.");
		return 1;
	}

	settings_start(state);
	dc_settings_start(state);

	// These dc values are const char *, therefore we have to cast.
	// Will be fixed by converting to std::string
	utf8_string(data[1], (char **)&state->cur_dive->dc.serial);
	utf8_string(data[12], (char **)&state->cur_dive->dc.fw_version);
	state->cur_dive->dc.model = strdup("Seac Action");

	state->cur_dive->dc.deviceid = calculate_string_hash(data[1]);

	add_extra_data(&state->cur_dive->dc, "GF-Lo", (const char*)sqlite3_column_text(sqlstmt, 9));
	add_extra_data(&state->cur_dive->dc, "GF-Hi", (const char*)sqlite3_column_text(sqlstmt, 10));

	dc_settings_end(state);
	settings_end(state);

	if (data[11]) {
		state->cur_dive->dc.maxdepth.mm = 10 * atoi(data[11]);
	}

	curcyl->gasmix.o2.permille = 10 * sqlite3_column_int(sqlstmt, 4);


	// Track gasses to tell when switch occurs
	lastgas = curcyl->gasmix;
	curgas = curcyl->gasmix;

	// Read samples
	while (retval == SQLITE_ROW) {
		sample_start(state);
		state->cur_sample->time.seconds = sqlite3_column_int(sqlstmt, 1);
		state->cur_sample->depth.mm = 10 * sqlite3_column_int(sqlstmt, 2);
		state->cur_sample->temperature.mkelvin = cC_to_mkelvin(sqlite3_column_int(sqlstmt, 3));
		curgas.o2.permille = 10 * sqlite3_column_int(sqlstmt, 4);
		if (!same_gasmix(lastgas, curgas)) {
			seac_gaschange(state, sqlstmt);
			lastgas = curgas;
			cylnum ^= 1; // Only need to toggle between two cylinders
			curcyl = get_or_create_cylinder(state->cur_dive, cylnum);
			curcyl->gasmix.o2.permille = 10 * sqlite3_column_int(sqlstmt, 4);
		}
		state->cur_sample->stopdepth.mm = 10 * sqlite3_column_int(sqlstmt, 5);
		state->cur_sample->stoptime.seconds = sqlite3_column_int(sqlstmt, 6);
		state->cur_sample->ndl.seconds = sqlite3_column_int(sqlstmt, 7);
		state->cur_sample->cns = sqlite3_column_int(sqlstmt, 8);
		sample_end(state);
		retval = sqlite3_step(sqlstmt);
	}

	sqlite3_finalize(sqlstmt);
	dive_end(state);

	return SQLITE_OK;
}

/** Read SeacSync divesDB.db sqlite3 database into dive and samples.
 *
 * Each row returned in the query of headers_dive creates a new dive.
 * The callback function performs another SQL query on the other
 * table, to read in the sample values.
 */
extern "C" int parse_seac_buffer(sqlite3 *handle, const char *url, const char *, int, struct divelog *log)
{
	int retval;
	char *err = NULL;
	struct parser_state state;

	state.log = log;
	state.sql_handle = handle;

	const char *get_dives = "SELECT dive_number, device_sn, date, timezone, time, elapsed_surface_time, dive_type, start_mode, water_type, comment, total_dive_time, max_depth, firmware_version, dive_id FROM headers_dive";
		/*  0 = dive_number
		 *  1 = device_sn
		 *  2 = date
		 *  3 = timezone
		 *  4 = time
		 *  5 = elapsed_surface_time
		 *  6 = dive_type
		 *  7 = start_mode
		 *  8 = water_type
		 *  9 = comment
		 * 10 = total_dive_time
		 * 11 = max_depth
		 * 12 = firmware version
		 * 13 = dive_id
		 */

	retval = sqlite3_exec(handle, get_dives, &seac_dive, &state, &err);

	if (retval != SQLITE_OK) {
		report_info("Database query failed '%s'.", url);
		return 1;
	}

	return 0;
}
