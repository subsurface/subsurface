// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include "dive.h"
#include "sample.h"
#include "subsurface-string.h"
#include "parse.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"

#include <stdlib.h>

static int shearwater_cylinders(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	int o2 = lrint(strtod_flags(data[0], NULL, 0) * 1000);
	int he = lrint(strtod_flags(data[1], NULL, 0) * 1000);

	/* Shearwater allows entering only 99%, not 100%
	 * so assume 99% to be pure oxygen */
	if (o2 == 990 && he == 0)
		o2 = 1000;

	cyl = cylinder_start(state);
	cyl->gasmix.o2.permille = o2;
	cyl->gasmix.he.permille = he;
	cylinder_end(state);

	return 0;
}

static int shearwater_changes(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	if (columns != 3) {
		return 1;
	}
	if (!data[0] || !data[1] || !data[2]) {
		return 2;
	}
	int o2 = lrint(strtod_flags(data[1], NULL, 0) * 1000);
	int he = lrint(strtod_flags(data[2], NULL, 0) * 1000);

	/* Shearwater allows entering only 99%, not 100%
	 * so assume 99% to be pure oxygen */
	if (o2 == 990 && he == 0)
		o2 = 1000;

	// Find the cylinder index
	int index;
	bool found = false;
	for (index = 0; index < state->cur_dive->cylinders.nr; ++index) {
		const cylinder_t *cyl = get_cylinder(state->cur_dive, index);
		if (cyl->gasmix.o2.permille == o2 && cyl->gasmix.he.permille == he) {
			found = true;
			break;
		}
	}
	if (!found) {
		// Cylinder not found, creating a new one
		cyl = cylinder_start(state);
		cyl->gasmix.o2.permille = o2;
		cyl->gasmix.he.permille = he;
		cylinder_end(state);
	}

	add_gas_switch_event(state->cur_dive, get_dc(state), state->sample_rate ? atoi(data[0]) / state->sample_rate * 10 : atoi(data[0]), index);
	return 0;
}

static int shearwater_profile_sample(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	int d6, d7;

	sample_start(state);

	/*
	 * If we have sample_rate, we use self calculated sample number
	 * to count the sample time.
	 * If we do not have sample_rate, we try to use the sample time
	 * provided by Shearwater as is.
	 */

	if (data[9] && state->sample_rate)
		state->cur_sample->time.seconds = atoi(data[9]) * state->sample_rate;
	else if (data[0])
		state->cur_sample->time.seconds = atoi(data[0]);


	if (data[1])
		state->cur_sample->depth.mm = state->metric ? lrint(strtod_flags(data[1], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[1], NULL, 0));
	if (data[2])
		state->cur_sample->temperature.mkelvin = state->metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	if (data[3]) {
		state->cur_sample->setpoint.mbar = lrint(strtod_flags(data[3], NULL, 0) * 1000);
	}
	if (data[4])
		state->cur_sample->ndl.seconds = atoi(data[4]) * 60;
	if (data[5])
		state->cur_sample->cns = atoi(data[5]);
	if (data[6]) {
		d6 = atoi(data[6]);
		if (d6 > 0) {
			state->cur_sample->stopdepth.mm = state->metric ? d6 * 1000 : feet_to_mm(d6);
			state->cur_sample->in_deco = 1;
		} else if (data[7]) {
			d7 = atoi(data[7]);
			if (d7 > 0) {
				state->cur_sample->stopdepth.mm = state->metric ? d7 * 1000 : feet_to_mm(d7);
				if (data[8])
					state->cur_sample->stoptime.seconds = atoi(data[8]) * 60;
				state->cur_sample->in_deco = 1;
			} else {
				state->cur_sample->in_deco = 0;
			}
		} else {
			state->cur_sample->in_deco = 0;
		}
	}

	/* We don't actually have data[3], but it should appear in the
	 * SQL query at some point.
	if (data[3])
		state->cur_sample->pressure[0].mbar = state->metric ? atoi(data[3]) * 1000 : psi_to_mbar(atoi(data[3]));
	 */
	sample_end(state);

	return 0;
}

static int shearwater_ai_profile_sample(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	int d6, d9;

	sample_start(state);

	/*
	 * If we have sample_rate, we use self calculated sample number
	 * to count the sample time.
	 * If we do not have sample_rate, we try to use the sample time
	 * provided by Shearwater as is.
	 */

	if (data[11] && state->sample_rate)
		state->cur_sample->time.seconds = atoi(data[11]) * state->sample_rate;
	else if (data[0])
		state->cur_sample->time.seconds = atoi(data[0]);

	if (data[1])
		state->cur_sample->depth.mm = state->metric ? lrint(strtod_flags(data[1], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[1], NULL, 0));
	if (data[2])
		state->cur_sample->temperature.mkelvin = state->metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	if (data[3]) {
		state->cur_sample->setpoint.mbar = lrint(strtod_flags(data[3], NULL, 0) * 1000);
	}
	if (data[4])
		state->cur_sample->ndl.seconds = atoi(data[4]) * 60;
	if (data[5])
		state->cur_sample->cns = atoi(data[5]);
	if (data[6]) {
		d6 = atoi(data[6]);
		if (d6 > 0) {
			state->cur_sample->stopdepth.mm = state->metric ? d6 * 1000 : feet_to_mm(d6);
			state->cur_sample->in_deco = 1;
		} else if (data[9]) {
			d9 = atoi(data[9]);
			if (d9 > 0) {
				state->cur_sample->stopdepth.mm = state->metric ? d9 * 1000 : feet_to_mm(d9);
				if (data[10])
					state->cur_sample->stoptime.seconds = atoi(data[10]) * 60;
				state->cur_sample->in_deco = 1;
			} else {
				state->cur_sample->in_deco = 0;
			}
		} else {
			state->cur_sample->in_deco = 0;
		}
	}

	/*
	 * I have seen sample log where the sample pressure had to be multiplied by 2. However,
	 * currently this seems to be corrected in ShearWater, so we are no longer taking this into
	 * account.
	 *
	 * Also, missing values might be nowadays 8184, even though an old log I have received has
	 * 8190. Thus discarding values over 8180 here.
	 */

	if (data[7] && atoi(data[7]) < 8180) {
		state->cur_sample->pressure[0].mbar = psi_to_mbar(atoi(data[7]));
	}
	if (data[8] && atoi(data[8]) < 8180)
		state->cur_sample->pressure[1].mbar = psi_to_mbar(atoi(data[8]));
	sample_end(state);

	return 0;
}

static int shearwater_mode(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	if (data[0])
		state->cur_dive->dc.divemode = atoi(data[0]) == 0 ? CCR : OC;

	return 0;
}

static int shearwater_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);

	int retval = 0;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	char get_profile_template[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling,firstStopDepth,firstStopTime from dive_log_records where diveLogId=%ld";
	char get_profile_template_ai[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling,aiSensor0_PressurePSI,aiSensor1_PressurePSI,firstStopDepth,firstStopTime from dive_log_records where diveLogId = %ld";
	char get_cylinder_template[] = "select fractionO2,fractionHe from dive_log_records where diveLogId = %ld group by fractionO2,fractionHe";
	char get_changes_template[] = "select a.currentTime,a.fractionO2,a.fractionHe from dive_log_records as a,dive_log_records as b where (a.id - 1) = b.id and (a.fractionO2 != b.fractionO2 or a.fractionHe != b.fractionHe) and a.diveLogId=b.divelogId and a.diveLogId = %ld";
	char get_mode_template[] = "select distinct currentCircuitSetting from dive_log_records where diveLogId = %ld";
	char get_buffer[1024];

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));

	long int dive_id = atol(data[11]);

	if (data[2])
		add_dive_site(data[2], state->cur_dive, state);
	if (data[3])
		utf8_string(data[3], &state->cur_dive->buddy);
	if (data[4])
		utf8_string(data[4], &state->cur_dive->notes);

	state->metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		state->cur_dive->dc.maxdepth.mm = state->metric ? lrint(strtod_flags(data[6], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[6], NULL, 0));

	if (data[7])
		state->cur_dive->dc.duration.seconds = atoi(data[7]) * 60;

	if (data[8])
		state->cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[9])
		utf8_string(data[9], &state->cur_settings.dc.serial_nr);
	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_settings.dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			state->cur_settings.dc.model = strdup("Shearwater Predator");
			break;
		default:
			state->cur_settings.dc.model = strdup("Shearwater import");
			break;
		}
	}

	state->cur_settings.dc.deviceid = atoi(data[9]);

	dc_settings_end(state);
	settings_end(state);

	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_dive->dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			state->cur_dive->dc.model = strdup("Shearwater Predator");
			break;
		default:
			state->cur_dive->dc.model = strdup("Shearwater import");
			break;
		}
	}

	if (data[11]) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_mode_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_mode, state, NULL);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_mode failed.\n");
			return 1;
		}
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_cylinders failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_changes failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template_ai, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_ai_profile_sample, state, NULL);
	if (retval != SQLITE_OK) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, state, NULL);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_profile_sample failed.\n");
			return 1;
		}
	}

	dive_end(state);

	return SQLITE_OK;
}

static int shearwater_cloud_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);

	int retval = 0;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;

	/*
	 * Since Shearwater reported sample time can be totally bogus,
	 * we need to calculate the sample number by ourselves. The
	 * calculated sample number is multiplied by sample interval
	 * giving us correct sample time.
	 */

	char get_profile_template[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling,firstStopDepth,firstStopTime,(select count(0) from dive_log_records r where r.id < d.id and r.diveLogId = %ld and r.currentTime > 0) as row from dive_log_records d where d.diveLogId=%ld and d.currentTime > 0";
	char get_profile_template_ai[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling,aiSensor0_PressurePSI,aiSensor1_PressurePSI,firstStopDepth,firstStopTime,(select count(0) from dive_log_records r where r.id < d.id and r.diveLogId = %ld and r.currentTime > 0) as row from dive_log_records d where d.diveLogId = %ld and d.currentTime > 0";

	char get_cylinder_template[] = "select fractionO2 / 100,fractionHe / 100 from dive_log_records where diveLogId = %ld group by fractionO2,fractionHe";
	char get_first_gas_template[] = "select currentTime, fractionO2 / 100, fractionHe / 100 from dive_log_records where diveLogId = %ld limit 1";
	char get_changes_template[] = "select a.currentTime,a.fractionO2 / 100,a.fractionHe /100 from dive_log_records as a,dive_log_records as b where (a.id - 1) = b.id and (a.fractionO2 != b.fractionO2 or a.fractionHe != b.fractionHe) and a.diveLogId=b.divelogId and a.diveLogId = %ld and a.fractionO2 > 0 and b.fractionO2 > 0";
	char get_mode_template[] = "select distinct currentCircuitSetting from dive_log_records where diveLogId = %ld";
	char get_buffer[1024];

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));

	long int dive_id = atol(data[11]);
	if (data[12])
		state->sample_rate = atoi(data[12]);
	else
		state->sample_rate = 0;

	if (data[2])
		add_dive_site(data[2], state->cur_dive, state);
	if (data[3])
		utf8_string(data[3], &state->cur_dive->buddy);
	if (data[4])
		utf8_string(data[4], &state->cur_dive->notes);

	state->metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		state->cur_dive->dc.maxdepth.mm = state->metric ? lrint(strtod_flags(data[6], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[6], NULL, 0));

	if (data[7])
		state->cur_dive->dc.duration.seconds = atoi(data[7]);

	if (data[8])
		state->cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[9])
		utf8_string(data[9], &state->cur_settings.dc.serial_nr);
	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_settings.dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			state->cur_settings.dc.model = strdup("Shearwater Predator");
			break;
		default:
			state->cur_settings.dc.model = strdup("Shearwater import");
			break;
		}
	}

	state->cur_settings.dc.deviceid = atoi(data[9]);

	dc_settings_end(state);
	settings_end(state);

	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_dive->dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			state->cur_dive->dc.model = strdup("Shearwater Predator");
			break;
		default:
			state->cur_dive->dc.model = strdup("Shearwater import");
			break;
		}
	}

	if (data[11]) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_mode_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_mode, state, NULL);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_mode failed.\n");
			return 1;
		}
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_cylinders failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_first_gas_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_changes failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_changes failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template_ai, dive_id, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_ai_profile_sample, state, NULL);
	if (retval != SQLITE_OK) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, dive_id, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, state, NULL);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_profile_sample failed.\n");
			return 1;
		}
	}

	dive_end(state);

	return SQLITE_OK;
}

int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
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

	// So far have not seen any sample rate in Shearwater Desktop
	state.sample_rate = 0;

	char get_dives[] = "select l.number,timestamp,location||' / '||site,buddy,notes,imperialUnits,maxDepth,maxTime,startSurfacePressure,computerSerial,computerModel,i.diveId FROM dive_info AS i JOIN dive_logs AS l ON i.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_dive, &state, NULL);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}

int parse_shearwater_cloud_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
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

	char get_dives[] = "select l.number,strftime('%s', DiveDate),location||' / '||site,buddy,notes,imperialUnits,maxDepth,DiveLengthTime,startSurfacePressure,computerSerial,computerModel,d.diveId,l.sampleRateMs / 1000 FROM dive_details AS d JOIN dive_logs AS l ON d.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_cloud_dive, &state, NULL);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}
