// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include "dive.h"
#include "parse.h"
#include "sample.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"
#include "tag.h"

#include <stdlib.h>

static int dm4_events(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	event_start(state);
	if (data[1])
		state->cur_event.time.seconds = atoi(data[1]);

	if (data[2]) {
		switch (atoi(data[2])) {
		case 1:
			/* 1 Mandatory Safety Stop */
			strcpy(state->cur_event.name, "safety stop (mandatory)");
			break;
		case 3:
			/* 3 Deco */
			/* What is Subsurface's term for going to
				 * deco? */
			strcpy(state->cur_event.name, "deco");
			break;
		case 4:
			/* 4 Ascent warning */
			strcpy(state->cur_event.name, "ascent");
			break;
		case 5:
			/* 5 Ceiling broken */
			strcpy(state->cur_event.name, "violation");
			break;
		case 6:
			/* 6 Mandatory safety stop ceiling error */
			strcpy(state->cur_event.name, "violation");
			break;
		case 7:
			/* 7 Below deco floor */
			strcpy(state->cur_event.name, "below floor");
			break;
		case 8:
			/* 8 Dive time alarm */
			strcpy(state->cur_event.name, "divetime");
			break;
		case 9:
			/* 9 Depth alarm */
			strcpy(state->cur_event.name, "maxdepth");
			break;
		case 10:
		/* 10 OLF 80% */
		case 11:
			/* 11 OLF 100% */
			strcpy(state->cur_event.name, "OLF");
			break;
		case 12:
			/* 12 High pO₂ */
			strcpy(state->cur_event.name, "PO2");
			break;
		case 13:
			/* 13 Air time */
			strcpy(state->cur_event.name, "airtime");
			break;
		case 17:
			/* 17 Ascent warning */
			strcpy(state->cur_event.name, "ascent");
			break;
		case 18:
			/* 18 Ceiling error */
			strcpy(state->cur_event.name, "ceiling");
			break;
		case 19:
			/* 19 Surfaced */
			strcpy(state->cur_event.name, "surface");
			break;
		case 20:
			/* 20 Deco */
			strcpy(state->cur_event.name, "deco");
			break;
		case 22:
		case 32:
			/* 22 Mandatory safety stop violation */
			/* 32 Deep stop violation */
			strcpy(state->cur_event.name, "violation");
			break;
		case 30:
			/* Tissue level warning */
			strcpy(state->cur_event.name, "tissue warning");
			break;
		case 37:
			/* Tank pressure alarm */
			strcpy(state->cur_event.name, "tank pressure");
			break;
		case 257:
			/* 257 Dive active */
			/* This seems to be given after surface when
			 * descending again. */
			strcpy(state->cur_event.name, "surface");
			break;
		case 258:
			/* 258 Bookmark */
			if (data[3]) {
				strcpy(state->cur_event.name, "heading");
				state->cur_event.value = atoi(data[3]);
			} else {
				strcpy(state->cur_event.name, "bookmark");
			}
			break;
		case 259:
			/* Deep stop */
			strcpy(state->cur_event.name, "Deep stop");
			break;
		case 260:
			/* Deep stop */
			strcpy(state->cur_event.name, "Deep stop cleared");
			break;
		case 266:
			/* Mandatory safety stop activated */
			strcpy(state->cur_event.name, "safety stop (mandatory)");
			break;
		case 267:
			/* Mandatory safety stop deactivated */
			/* DM5 shows this only on event list, not on the
			 * profile so skipping as well for now */
			break;
		default:
			strcpy(state->cur_event.name, "unknown");
			state->cur_event.value = atoi(data[2]);
			break;
		}
	}
	event_end(state);

	return 0;
}

static int dm4_tags(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	if (data[0])
		taglist_add_tag(&state->cur_dive->tag_list, data[0]);

	return 0;
}

static int dm4_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	int i;
	int interval, retval = 0;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	float *profileBlob;
	unsigned char *tempBlob;
	int *pressureBlob;
	char get_events_template[] = "select * from Mark where DiveId = %d";
	char get_tags_template[] = "select Text from DiveTag where DiveId = %d";
	char get_events[64];
	cylinder_t *cyl;

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));
	if (data[2])
		utf8_string(data[2], &state->cur_dive->notes);

	/*
	 * DM4 stores Duration and DiveTime. It looks like DiveTime is
	 * 10 to 60 seconds shorter than Duration. However, I have no
	 * idea what is the difference and which one should be used.
	 * Duration = data[3]
	 * DiveTime = data[15]
	 */
	if (data[3])
		state->cur_dive->duration.seconds = atoi(data[3]);
	if (data[15])
		state->cur_dive->dc.duration.seconds = atoi(data[15]);

	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[4])
		utf8_string(data[4], &state->cur_settings.dc.serial_nr);
	if (data[5])
		utf8_string(data[5], &state->cur_settings.dc.model);

	state->cur_settings.dc.deviceid = 0xffffffff;
	dc_settings_end(state);
	settings_end(state);

	if (data[6])
		state->cur_dive->dc.maxdepth.mm = lrint(strtod_flags(data[6], NULL, 0) * 1000);
	if (data[8])
		state->cur_dive->dc.airtemp.mkelvin = C_to_mkelvin(atoi(data[8]));
	if (data[9])
		state->cur_dive->dc.watertemp.mkelvin = C_to_mkelvin(atoi(data[9]));

	/*
	 * TODO: handle multiple cylinders
	 */
	cyl = cylinder_start(state);
	if (data[22] && atoi(data[22]) > 0)
		cyl->start.mbar = atoi(data[22]);
	else if (data[10] && atoi(data[10]) > 0)
		cyl->start.mbar = atoi(data[10]);
	if (data[23] && atoi(data[23]) > 0)
		cyl->end.mbar = (atoi(data[23]));
	if (data[11] && atoi(data[11]) > 0)
		cyl->end.mbar = (atoi(data[11]));
	if (data[12])
		cyl->type.size.mliter = lrint((strtod_flags(data[12], NULL, 0)) * 1000);
	if (data[13])
		cyl->type.workingpressure.mbar = (atoi(data[13]));
	if (data[20])
		cyl->gasmix.o2.permille = atoi(data[20]) * 10;
	if (data[21])
		cyl->gasmix.he.permille = atoi(data[21]) * 10;
	cylinder_end(state);

	if (data[14])
		state->cur_dive->dc.surface_pressure.mbar = (atoi(data[14]) * 1000);

	interval = data[16] ? atoi(data[16]) : 0;
	profileBlob = (float *)data[17];
	tempBlob = (unsigned char *)data[18];
	pressureBlob = (int *)data[19];
	for (i = 0; interval && i * interval < state->cur_dive->duration.seconds; i++) {
		sample_start(state);
		state->cur_sample->time.seconds = i * interval;
		if (profileBlob)
			state->cur_sample->depth.mm = lrintf(profileBlob[i] * 1000.0f);
		else
			state->cur_sample->depth.mm = state->cur_dive->dc.maxdepth.mm;

		if (data[18] && data[18][0])
			state->cur_sample->temperature.mkelvin = C_to_mkelvin(tempBlob[i]);
		if (data[19] && data[19][0])
			state->cur_sample->pressure[0].mbar = pressureBlob[i];
		sample_end(state);
	}

	snprintf(get_events, sizeof(get_events) - 1, get_events_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_events, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm4_events failed.\n");
		return 1;
	}

	snprintf(get_events, sizeof(get_events) - 1, get_tags_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_tags, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm4_tags failed.\n");
		return 1;
	}

	dive_end(state);

	/*
	for (i=0; i<columns;++i) {
		fprintf(stderr, "%s\t", column[i]);
	}
	fprintf(stderr, "\n");
	for (i=0; i<columns;++i) {
		fprintf(stderr, "%s\t", data[i]);
	}
	fprintf(stderr, "\n");
	//exit(0);
	*/
	return SQLITE_OK;
}

int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
		     struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		     struct device_table *devices)
{
	UNUSED(buffer);
	UNUSED(size);

	int retval;
	char *err = NULL;
	struct parser_state state;

	init_parser_state(&state);
	state.target_table = table;
	state.trips = trips;
	state.sites = sites;
	state.devices = devices;
	state.sql_handle = handle;

	/* StartTime is converted from Suunto's nano seconds to standard
	 * time. We also need epoch, not seconds since year 1. */
	char get_dives[] = "select D.DiveId,StartTime/10000000-62135596800,Note,Duration,SourceSerialNumber,Source,MaxDepth,SampleInterval,StartTemperature,BottomTemperature,D.StartPressure,D.EndPressure,Size,CylinderWorkPressure,SurfacePressure,DiveTime,SampleInterval,ProfileBlob,TemperatureBlob,PressureBlob,Oxygen,Helium,MIX.StartPressure,MIX.EndPressure FROM Dive AS D JOIN DiveMixture AS MIX ON D.DiveId=MIX.DiveId";

	retval = sqlite3_exec(handle, get_dives, &dm4_dive, &state, &err);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}

static int dm5_cylinders(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	cyl = cylinder_start(state);
	if (data[7] && atoi(data[7]) > 0 && atoi(data[7]) < 350000)
		cyl->start.mbar = atoi(data[7]);
	if (data[8] && atoi(data[8]) > 0 && atoi(data[8]) < 350000)
		cyl->end.mbar = (atoi(data[8]));
	if (data[6]) {
		/* DM5 shows tank size of 12 liters when the actual
		 * value is 0 (and using metric units). So we just use
		 * the same 12 liters when size is not available */
		if (strtod_flags(data[6], NULL, 0) == 0.0 && cyl->start.mbar)
			cyl->type.size.mliter = 12000;
		else
			cyl->type.size.mliter = lrint((strtod_flags(data[6], NULL, 0)) * 1000);
	}
	if (data[2])
		cyl->gasmix.o2.permille = atoi(data[2]) * 10;
	if (data[3])
		cyl->gasmix.he.permille = atoi(data[3]) * 10;
	cylinder_end(state);
	return 0;
}

static int dm5_gaschange(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	event_start(state);
	if (data[0])
		state->cur_event.time.seconds = atoi(data[0]);
	if (data[1]) {
		strcpy(state->cur_event.name, "gaschange");
		state->cur_event.value = lrint(strtod_flags(data[1], NULL, 0));
	}

	/* He part of the mix */
	if (data[2])
		state->cur_event.value += lrint(strtod_flags(data[2], NULL, 0)) << 16;
	event_end(state);

	return 0;
}

static int dm5_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	int i;
	int tempformat = 0;
	int interval, retval = 0, block_size;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	unsigned const char *sampleBlob;
	char get_events_template[] = "select * from Mark where DiveId = %d";
	char get_tags_template[] = "select Text from DiveTag where DiveId = %d";
	char get_cylinders_template[] = "select * from DiveMixture where DiveId = %d";
	char get_gaschange_template[] = "select GasChangeTime,Oxygen,Helium from DiveGasChange join DiveMixture on DiveGasChange.DiveMixtureId=DiveMixture.DiveMixtureId where DiveId = %d";
	char get_events[512];

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));
	if (data[2])
		utf8_string(data[2], &state->cur_dive->notes);

	if (data[3])
		state->cur_dive->duration.seconds = atoi(data[3]);
	if (data[15])
		state->cur_dive->dc.duration.seconds = atoi(data[15]);

	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[4]) {
		utf8_string(data[4], &state->cur_settings.dc.serial_nr);
		state->cur_settings.dc.deviceid = atoi(data[4]);
	}
	if (data[5])
		utf8_string(data[5], &state->cur_settings.dc.model);

	dc_settings_end(state);
	settings_end(state);

	if (data[6])
		state->cur_dive->dc.maxdepth.mm = lrint(strtod_flags(data[6], NULL, 0) * 1000);
	if (data[8])
		state->cur_dive->dc.airtemp.mkelvin = C_to_mkelvin(atoi(data[8]));
	if (data[9])
		state->cur_dive->dc.watertemp.mkelvin = C_to_mkelvin(atoi(data[9]));

	if (data[4]) {
		state->cur_dive->dc.deviceid = atoi(data[4]);
	}
	if (data[5])
		utf8_string(data[5], &state->cur_dive->dc.model);

	snprintf(get_events, sizeof(get_events) - 1, get_cylinders_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm5_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm5_cylinders failed.\n");
		return 1;
	}

	if (data[14])
		state->cur_dive->dc.surface_pressure.mbar = (atoi(data[14]) / 100);

	interval = data[16] ? atoi(data[16]) : 0;

	/*
	 * sampleBlob[0]	version number, indicates the size of one sample
	 *
	 * Following ones describe single sample, bugs in interpretation of the binary blob are likely:
	 *
	 * sampleBlob[3]	depth
	 * sampleBlob[7-9]	pressure
	 * sampleBlob[11]	temperature - either full Celsius or float, might be different field for some version of DM
	 */

	sampleBlob = (unsigned const char *)data[24];

	if (sampleBlob) {
		switch (sampleBlob[0]) {
			case 1:
				// Log is converted from DM4 to DM5
				block_size = 16;
				break;
			case 2:
				block_size = 19;
				break;
			case 3:
				block_size = 23;
				break;
			case 4:
				// Temperature is stored in float
				tempformat = 1;
				block_size = 26;
				break;
			case 5:
				// Temperature is stored in float
				tempformat = 1;
				block_size = 30;
				break;
			default:
				block_size = 16;
				break;
		}
	}

	for (i = 0; interval && sampleBlob && i * interval < state->cur_dive->duration.seconds; i++) {
		float *depth = (float *)&sampleBlob[i * block_size + 3];
		int32_t pressure = (sampleBlob[i * block_size + 9] << 16) + (sampleBlob[i * block_size + 8] << 8) + sampleBlob[i * block_size + 7];

		sample_start(state);
		state->cur_sample->time.seconds = i * interval;
		state->cur_sample->depth.mm = lrintf(depth[0] * 1000.0f);

		if (tempformat == 1) {
			float *temp = (float *)&(sampleBlob[i * block_size + 11]);
			state->cur_sample->temperature.mkelvin = C_to_mkelvin(*temp);
		} else {
			if ((sampleBlob[i * block_size + 11]) != 0x7F) {
				state->cur_sample->temperature.mkelvin = C_to_mkelvin(sampleBlob[i * block_size + 11]);
			}
		}

		/*
		 * Limit cylinder pressures to somewhat sensible values
		 */
		if (pressure >= 0 && pressure < 350000)
			state->cur_sample->pressure[0].mbar = pressure;
		sample_end(state);
	}

	/*
	 * Log was converted from DM4, thus we need to parse the profile
	 * from DM4 format
	 */

	if (i == 0) {
		float *profileBlob;
		unsigned char *tempBlob;
		int *pressureBlob;

		profileBlob = (float *)data[17];
		tempBlob = (unsigned char *)data[18];
		pressureBlob = (int *)data[19];
		for (i = 0; interval && i * interval < state->cur_dive->duration.seconds; i++) {
			sample_start(state);
			state->cur_sample->time.seconds = i * interval;
			if (profileBlob)
				state->cur_sample->depth.mm = lrintf(profileBlob[i] * 1000.0f);
			else
				state->cur_sample->depth.mm = state->cur_dive->dc.maxdepth.mm;

			if (data[18] && data[18][0])
				state->cur_sample->temperature.mkelvin = C_to_mkelvin(tempBlob[i]);
			if (data[19] && data[19][0])
				state->cur_sample->pressure[0].mbar = pressureBlob[i];
			sample_end(state);
		}
	}

	snprintf(get_events, sizeof(get_events) - 1, get_gaschange_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm5_gaschange, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm5_gaschange failed.\n");
		return 1;
	}

	snprintf(get_events, sizeof(get_events) - 1, get_events_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_events, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm4_events failed.\n");
		return 1;
	}

	snprintf(get_events, sizeof(get_events) - 1, get_tags_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_tags, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query dm4_tags failed.\n");
		return 1;
	}

	dive_end(state);

	return SQLITE_OK;
}

int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
		     struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		     struct device_table *devices)
{
	UNUSED(buffer);
	UNUSED(size);

	int retval;
	char *err = NULL;
	struct parser_state state;

	init_parser_state(&state);
	state.target_table = table;
	state.trips = trips;
	state.sites = sites;
	state.devices = devices;
	state.sql_handle = handle;

	/* StartTime is converted from Suunto's nano seconds to standard
	 * time. We also need epoch, not seconds since year 1. */
	char get_dives[] = "select DiveId,StartTime/10000000-62135596800,Note,Duration,coalesce(SourceSerialNumber,SerialNumber),Source,MaxDepth,SampleInterval,StartTemperature,BottomTemperature,StartPressure,EndPressure,'','',SurfacePressure,DiveTime,SampleInterval,ProfileBlob,TemperatureBlob,PressureBlob,'','','','',SampleBlob FROM Dive where Deleted is null";

	retval = sqlite3_exec(handle, get_dives, &dm5_dive, &state, &err);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}

