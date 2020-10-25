// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include "dive.h"
#include "divesite.h"
#include "sample.h"
#include "subsurface-string.h"
#include "parse.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"

static int divinglog_cylinder(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	short dbl = 1;
	//char get_cylinder_template[] = "select TankID,TankSize,PresS,PresE,PresW,O2,He,DblTank from Tank where LogID = %d";

	if (data[7] && atoi(data[7]) > 0)
		dbl = 2;

	cyl = cylinder_start(state);

	/*
	 * Assuming that we have to double the cylinder size, if double
	 * is set
	 */

	if (data[1] && atoi(data[1]) > 0)
		cyl->type.size.mliter = atol(data[1]) * 1000 * dbl;

	if (data[2] && atoi(data[2]) > 0)
		cyl->start.mbar = atol(data[2]) * 1000;
	if (data[3] && atoi(data[3]) > 0)
		cyl->end.mbar = atol(data[3]) * 1000;
	if (data[4] && atoi(data[4]) > 0)
		cyl->type.workingpressure.mbar = atol(data[4]) * 1000;
	if (data[5] && atoi(data[5]) > 0)
		cyl->gasmix.o2.permille = atol(data[5]) * 10;
	if (data[6] && atoi(data[6]) > 0)
		cyl->gasmix.he.permille = atol(data[6]) * 10;

	cylinder_end(state);

	return 0;
}

static int divinglog_profile(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	int sinterval = 0;
	unsigned long time;
	int len1, len2, len3, len4, len5;
	char *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
	short oldcyl = -1;

	/* We do not have samples */
	if (!data[1])
		return 0;

	if (data[0])
		sinterval = atoi(data[0]);

	/*
	 * Profile
	 *
	 * DDDDDCRASWEE
	 * D: Depth (in meter with two decimals)
	 * C: Deco (1 = yes, 0 = no)
	 * R: RBT (Remaining Bottom Time warning)
	 * A: Ascent warning
	 * S: Decostop ignored
	 * W: Work warning
	 * E: Extra info (different for every computer)
	 *
	 * Example: 004500010000
	 * 4.5 m, no deco, no RBT warning, ascanding too fast, no decostop ignored, no work, no extra info
	 *
	 *
	 * Profile2
	 *
	 * TTTFFFFIRRR
	 *
	 * T: Temperature (in °C with one decimal)
	 * F: Tank pressure 1 (in bar with one decimal)
	 * I: Tank ID (0, 1, 2 ... 9)
	 * R: RBT (in min)
	 *
	 * Example: 25518051099
	 * 25.5 °C, 180.5 bar, Tank 1, 99 min RBT
	 *
	 */

	ptr1 = data[1];
	ptr2 = data[2];
	ptr3 = data[3];
	ptr4 = data[4];
	ptr5 = data[5];
	len1 = strlen(ptr1);
	len2 = ptr2 ? strlen(ptr2) : 0;
	len3 = ptr3 ? strlen(ptr3) : 0;
	len4 = ptr4 ? strlen(ptr4) : 0;
	len5 = ptr5 ? strlen(ptr5) : 0;

	time = 0;
	while (len1 >= 12) {
		sample_start(state);

		state->cur_sample->time.seconds = time;
		state->cur_sample->in_deco = ptr1[5] - '0' ? true : false;
		state->cur_sample->depth.mm = atoi_n(ptr1, 5) * 10;

		if (len2 >= 11) {
			int temp = atoi_n(ptr2, 3);
			int pressure = atoi_n(ptr2+3, 4);
			int tank = atoi_n(ptr2+7, 1);
			int rbt = atoi_n(ptr2+8, 3) * 60;

			state->cur_sample->temperature.mkelvin = C_to_mkelvin(temp / 10.0f);
			state->cur_sample->pressure[0].mbar = pressure * 100;
			state->cur_sample->rbt.seconds = rbt;
			if (oldcyl != tank && tank >= 0 && tank < state->cur_dive->cylinders.nr) {
				struct gasmix mix = get_cylinder(state->cur_dive, tank)->gasmix;
				int o2 = get_o2(mix);
				int he = get_he(mix);

				event_start(state);
				state->cur_event.time.seconds = time;
				strcpy(state->cur_event.name, "gaschange");

				o2 = (o2 + 5) / 10;
				he = (he + 5) / 10;
				state->cur_event.value = o2 + (he << 16);

				event_end(state);
				oldcyl = tank;
			}

			ptr2 += 11; len2 -= 11;
		}

		if (len3 >= 14) {
			state->cur_sample->heartbeat = atoi_n(ptr3+8, 3);
			ptr3 += 14; len3 -= 14;
		}

		if (len4 >= 9) {
			/*
			 * Following value is NDL when not in deco, and
			 * either 0 or TTS when in deco.
			 */
			int val = atoi_n(ptr4, 3);
			if (state->cur_sample->in_deco) {
				state->cur_sample->ndl.seconds = 0;
				if (val)
					state->cur_sample->tts.seconds = val * 60;
			} else {
				state->cur_sample->ndl.seconds = val * 60;
			}
			state->cur_sample->stoptime.seconds = atoi_n(ptr4+3, 3) * 60;
			state->cur_sample->stopdepth.mm = atoi_n(ptr4+6, 3) * 1000;
			ptr4 += 9; len4 -= 9;
		}

		/*
		 * AAABBBCCCOOOONNNNSS
		 *
		 * A = ppO2 cell 1 (measured)
		 * B = ppO2 cell 2 (measured)
		 * C = ppO2 cell 3 (measured)
		 * O = OTU
		 * N = CNS
		 * S = Setpoint
		 *
		 * Example: 1121131141548026411
		 * 1.12 bar, 1.13 bar, 1.14 bar, OTU = 154.8, CNS = 26.4, Setpoint = 1.1
		 */

		if (len5 >= 19) {
			int ppo2_1 = atoi_n(ptr5 + 0, 3);
			int ppo2_2 = atoi_n(ptr5 + 3, 3);
			int ppo2_3 = atoi_n(ptr5 + 6, 3);
			int otu = atoi_n(ptr5 + 9, 4);
			UNUSED(otu); // we seem to not store this? Do we understand its format?
			int cns = atoi_n(ptr5 + 13, 4);
			int setpoint = atoi_n(ptr5 + 17, 2);

			if (ppo2_1 > 0)
				state->cur_sample->o2sensor[0].mbar = ppo2_1 * 100;
			if (ppo2_2 > 0)
				state->cur_sample->o2sensor[1].mbar = ppo2_2 * 100;
			if (ppo2_3 > 0)
				state->cur_sample->o2sensor[2].mbar = ppo2_3 * 100;
			if (cns > 0)
				state->cur_sample->cns = lrintf(cns / 10.0f);
			if (setpoint > 0)
				state->cur_sample->setpoint.mbar = setpoint * 100;
			ptr5 += 19; len5 -= 19;
		}

		/*
		 * Count the number of o2 sensors
		 */

		if (!state->cur_dive->dc.no_o2sensors && (state->cur_sample->o2sensor[0].mbar || state->cur_sample->o2sensor[1].mbar || state->cur_sample->o2sensor[2].mbar)) {
			state->cur_dive->dc.no_o2sensors = state->cur_sample->o2sensor[0].mbar ? 1 : 0 +
				 state->cur_sample->o2sensor[1].mbar ? 1 : 0 +
				 state->cur_sample->o2sensor[2].mbar ? 1 : 0;
		}

		sample_end(state);

		/* Remaining bottom time warning */
		if (ptr1[6] - '0') {
			event_start(state);
			state->cur_event.time.seconds = time;
			strcpy(state->cur_event.name, "rbt");
			event_end(state);
		}

		/* Ascent warning */
		if (ptr1[7] - '0') {
			event_start(state);
			state->cur_event.time.seconds = time;
			strcpy(state->cur_event.name, "ascent");
			event_end(state);
		}

		/* Deco stop ignored */
		if (ptr1[8] - '0') {
			event_start(state);
			state->cur_event.time.seconds = time;
			strcpy(state->cur_event.name, "violation");
			event_end(state);
		}

		/* Workload warning */
		if (ptr1[9] - '0') {
			event_start(state);
			state->cur_event.time.seconds = time;
			strcpy(state->cur_event.name, "workload");
			event_end(state);
		}

		ptr1 += 12; len1 -= 12;
		time += sinterval;
	}

	return 0;
}

static int divinglog_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);

	int retval = 0, diveid;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	char get_profile_template[] = "select ProfileInt,Profile,Profile2,Profile3,Profile4,Profile5 from Logbook where ID = %d";
	char get_cylinder0_template[] = "select 0,TankSize,PresS,PresE,PresW,O2,He,DblTank from Logbook where ID = %d";
	char get_cylinder_template[] = "select TankID,TankSize,PresS,PresE,PresW,O2,He,DblTank from Tank where LogID = %d order by TankID";
	char get_buffer[1024];

	dive_start(state);
	diveid = atoi(data[13]);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));

	if (data[2])
		add_dive_to_dive_site(state->cur_dive, find_or_create_dive_site_with_name(data[2], state->sites));

	if (data[3])
		utf8_string(data[3], &state->cur_dive->buddy);

	if (data[4])
		utf8_string(data[4], &state->cur_dive->notes);

	if (data[5])
		state->cur_dive->dc.maxdepth.mm = lrint(strtod_flags(data[5], NULL, 0) * 1000);

	if (data[6])
		state->cur_dive->dc.duration.seconds = atoi(data[6]) * 60;

	if (data[7])
		utf8_string(data[7], &state->cur_dive->divemaster);

	if (data[8])
		state->cur_dive->airtemp.mkelvin = C_to_mkelvin(atol(data[8]));

	if (data[9])
		state->cur_dive->watertemp.mkelvin = C_to_mkelvin(atol(data[9]));

	if (data[10]) {
		weightsystem_t ws = { { atol(data[10]) * 1000 }, translate("gettextFromC", "unknown") };
		add_cloned_weightsystem(&state->cur_dive->weightsystems, ws);
	}

	if (data[11])
		state->cur_dive->suit = strdup(data[11]);

	/* Divinglog has following visibility options: good, medium, bad */
	if (data[14]) {
		switch(data[14][0]) {
		case '0':
			break;
		case '1':
			state->cur_dive->visibility = 5;
			break;
		case '2':
			state->cur_dive->visibility = 3;
			break;
		case '3':
			state->cur_dive->visibility = 1;
			break;
		default:
			break;
		}
	}

	settings_start(state);
	dc_settings_start(state);

	if (data[12]) {
		state->cur_dive->dc.model = strdup(data[12]);
	} else {
		state->cur_settings.dc.model = strdup("Divinglog import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder0_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_cylinder, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query divinglog_cylinder0 failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_cylinder, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query divinglog_cylinder failed.\n");
		return 1;
	}

	if (data[15]) {
		switch (data[15][0]) {
		/* OC */
		case '0':
			break;
		case '1':
			state->cur_dive->dc.divemode = PSCR;
			break;
		case '2':
			state->cur_dive->dc.divemode = CCR;
			break;
		}
	}

	dc_settings_end(state);
	settings_end(state);

	if (data[12]) {
		state->cur_dive->dc.model = strdup(data[12]);
	} else {
		state->cur_dive->dc.model = strdup("Divinglog import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_profile, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query divinglog_profile failed.\n");
		return 1;
	}

	dive_end(state);

	return SQLITE_OK;
}


int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
			    struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
			    struct device_table *devices)
{
	UNUSED(buffer);
	UNUSED(size);

	int retval;
	struct parser_state state;

	init_parser_state(&state);
	state.target_table = table;
	state.trips = trips;
	state.sites = sites;
	state.devices = devices;
	state.sql_handle = handle;

	char get_dives[] = "select Number,strftime('%s',Divedate || ' ' || ifnull(Entrytime,'00:00')),Country || ' - ' || City || ' - ' || Place,Buddy,Comments,Depth,Divetime,Divemaster,Airtemp,Watertemp,Weight,Divesuit,Computer,ID,Visibility,SupplyType from Logbook where UUID not in (select UUID from DeletedRecords)";

	retval = sqlite3_exec(handle, get_dives, &divinglog_dive, &state, NULL);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}
