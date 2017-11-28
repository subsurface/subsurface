// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "dive.h"
#include "parse.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"

extern int shearwater_cylinders(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	int o2 = lrint(strtod_flags(data[0], NULL, 0) * 1000);
	int he = lrint(strtod_flags(data[1], NULL, 0) * 1000);

	/* Shearwater allows entering only 99%, not 100%
	 * so assume 99% to be pure oxygen */
	if (o2 == 990 && he == 0)
		o2 = 1000;

	cylinder_start();
	cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = o2;
	cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = he;
	cylinder_end();

	return 0;
}

extern int shearwater_changes(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

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
	int i;
	bool found = false;
	for (i = 0; i < cur_cylinder_index; ++i) {
		if (cur_dive->cylinder[i].gasmix.o2.permille == o2 && cur_dive->cylinder[i].gasmix.he.permille == he) {
			found = true;
			break;
		}
	}
	if (!found) {
		// Cylinder not found, creating a new one
		cylinder_start();
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = o2;
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = he;
		cylinder_end();
		i = cur_cylinder_index;
	}

	add_gas_switch_event(cur_dive, get_dc(), atoi(data[0]), i);
	return 0;
}

extern int shearwater_profile_sample(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	sample_start();
	if (data[0])
		cur_sample->time.seconds = atoi(data[0]);
	if (data[1])
		cur_sample->depth.mm = metric ? lrint(strtod_flags(data[1], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[1], NULL, 0));
	if (data[2])
		cur_sample->temperature.mkelvin = metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	if (data[3]) {
		cur_sample->setpoint.mbar = lrint(strtod_flags(data[3], NULL, 0) * 1000);
	}
	if (data[4])
		cur_sample->ndl.seconds = atoi(data[4]) * 60;
	if (data[5])
		cur_sample->cns = atoi(data[5]);
	if (data[6])
		cur_sample->stopdepth.mm = metric ? atoi(data[6]) * 1000 : feet_to_mm(atoi(data[6]));

	/* We don't actually have data[3], but it should appear in the
	 * SQL query at some point.
	if (data[3])
		cur_sample->pressure[0].mbar = metric ? atoi(data[3]) * 1000 : psi_to_mbar(atoi(data[3]));
	 */
	sample_end();

	return 0;
}

extern int shearwater_ai_profile_sample(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	sample_start();
	if (data[0])
		cur_sample->time.seconds = atoi(data[0]);
	if (data[1])
		cur_sample->depth.mm = metric ? lrint(strtod_flags(data[1], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[1], NULL, 0));
	if (data[2])
		cur_sample->temperature.mkelvin = metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	if (data[3]) {
		cur_sample->setpoint.mbar = lrint(strtod_flags(data[3], NULL, 0) * 1000);
	}
	if (data[4])
		cur_sample->ndl.seconds = atoi(data[4]) * 60;
	if (data[5])
		cur_sample->cns = atoi(data[5]);
	if (data[6])
		cur_sample->stopdepth.mm = metric ? atoi(data[6]) * 1000 : feet_to_mm(atoi(data[6]));

	/* Weird unit conversion but seems to produce correct results.
	 * Also missing values seems to be reported as a 4092 (564 bar) */
	if (data[7] && atoi(data[7]) != 4092) {
		cur_sample->pressure[0].mbar = psi_to_mbar(atoi(data[7])) * 2;
	}
	if (data[8] && atoi(data[8]) != 4092)
		cur_sample->pressure[1].mbar = psi_to_mbar(atoi(data[8])) * 2;
	sample_end();

	return 0;
}

extern int shearwater_mode(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	if (data[0])
		cur_dive->dc.divemode = atoi(data[0]) == 0 ? CCR : OC;

	return 0;
}

extern int shearwater_dive(void *param, int columns, char **data, char **column)
{
	(void) columns;
	(void) column;

	int retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	char *err = NULL;
	char get_profile_template[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling from dive_log_records where diveLogId=%d";
	char get_profile_template_ai[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling,aiSensor0_PressurePSI,aiSensor1_PressurePSI from dive_log_records where diveLogId = %d";
	char get_cylinder_template[] = "select fractionO2,fractionHe from dive_log_records where diveLogId = %d group by fractionO2,fractionHe";
	char get_changes_template[] = "select a.currentTime,a.fractionO2,a.fractionHe from dive_log_records as a,dive_log_records as b where (a.id - 1) = b.id and (a.fractionO2 != b.fractionO2 or a.fractionHe != b.fractionHe) and a.diveLogId=b.divelogId and a.diveLogId = %d";
	char get_mode_template[] = "select distinct currentCircuitSetting from dive_log_records where diveLogId = %d";
	char get_buffer[1024];

	dive_start();
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));

	int dive_id = atoi(data[11]);

	if (data[2])
		add_dive_site(data[2], cur_dive);
	if (data[3])
		utf8_string(data[3], &cur_dive->buddy);
	if (data[4])
		utf8_string(data[4], &cur_dive->notes);

	metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		cur_dive->dc.maxdepth.mm = metric ? lrint(strtod_flags(data[6], NULL, 0) * 1000) : feet_to_mm(strtod_flags(data[6], NULL, 0));

	if (data[7])
		cur_dive->dc.duration.seconds = atoi(data[7]) * 60;

	if (data[8])
		cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start();
	dc_settings_start();
	if (data[9])
		utf8_string(data[9], &cur_settings.dc.serial_nr);
	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			cur_settings.dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			cur_settings.dc.model = strdup("Shearwater Predator");
			break;
		default:
			cur_settings.dc.model = strdup("Shearwater import");
			break;
		}
	}

	cur_settings.dc.deviceid = atoi(data[9]);

	dc_settings_end();
	settings_end();

	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			cur_dive->dc.model = strdup("Shearwater Petrel/Perdix");
			break;
		case 4:
			cur_dive->dc.model = strdup("Shearwater Predator");
			break;
		default:
			cur_dive->dc.model = strdup("Shearwater import");
			break;
		}
	}

	if (data[11]) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_mode_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_mode, 0, &err);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_mode failed.\n");
			return 1;
		}
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_cylinders failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query shearwater_changes failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template_ai, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_ai_profile_sample, 0, &err);
	if (retval != SQLITE_OK) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, 0, &err);
		if (retval != SQLITE_OK) {
			fprintf(stderr, "%s", "Database query shearwater_profile_sample failed.\n");
			return 1;
		}
	}

	dive_end();

	return SQLITE_OK;
}

int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
			    struct dive_table *table)
{
	(void) buffer;
	(void) size;

	int retval;
	char *err = NULL;
	target_table = table;

	char get_dives[] = "select l.number,timestamp,location||' / '||site,buddy,notes,imperialUnits,maxDepth,maxTime,startSurfacePressure,computerSerial,computerModel,i.diveId FROM dive_info AS i JOIN dive_logs AS l ON i.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}

