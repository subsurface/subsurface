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

extern int cobalt_profile_sample(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	sample_start();
	if (data[0])
		cur_sample->time.seconds = atoi(data[0]);
	if (data[1])
		cur_sample->depth.mm = atoi(data[1]);
	if (data[2])
		cur_sample->temperature.mkelvin = metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	sample_end();

	return 0;
}


extern int cobalt_cylinders(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	cylinder_start();
	if (data[0])
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = atoi(data[0]) * 10;
	if (data[1])
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = atoi(data[1]) * 10;
	if (data[2])
		cur_dive->cylinder[cur_cylinder_index].start.mbar = psi_to_mbar(atoi(data[2]));
	if (data[3])
		cur_dive->cylinder[cur_cylinder_index].end.mbar = psi_to_mbar(atoi(data[3]));
	if (data[4])
		cur_dive->cylinder[cur_cylinder_index].type.size.mliter = atoi(data[4]) * 100;
	if (data[5])
		cur_dive->cylinder[cur_cylinder_index].gas_used.mliter = atoi(data[5]) * 1000;
	cylinder_end();

	return 0;
}

extern int cobalt_buddies(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	if (data[0])
		utf8_string(data[0], &cur_dive->buddy);

	return 0;
}

/*
 * We still need to figure out how to map free text visibility to
 * Subsurface star rating.
 */

extern int cobalt_visibility(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;
	(void) data;
	return 0;
}

extern int cobalt_location(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	static char *location = NULL;
	if (data[0]) {
		if (location) {
			char *tmp = malloc(strlen(location) + strlen(data[0]) + 4);
			if (!tmp)
				return -1;
			sprintf(tmp, "%s / %s", location, data[0]);
			free(location);
			location = NULL;
			cur_dive->dive_site_uuid = find_or_create_dive_site_with_name(tmp, cur_dive->when);
			free(tmp);
		} else {
			location = strdup(data[0]);
		}
	}
	return 0;
}


extern int cobalt_dive(void *param, int columns, char **data, char **column)
{
	(void) columns;
	(void) column;

	int retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	char *err = NULL;
	char get_profile_template[] = "select runtime*60,(DepthPressure*10000/SurfacePressure)-10000,p.Temperature from Dive AS d JOIN TrackPoints AS p ON d.Id=p.DiveId where d.Id=%d";
	char get_cylinder_template[] = "select FO2,FHe,StartingPressure,EndingPressure,TankSize,TankPressure,TotalConsumption from GasMixes where DiveID=%d and StartingPressure>0 and EndingPressure > 0 group by FO2,FHe";
	char get_buddy_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=4";
	char get_visibility_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=3";
	char get_location_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=0";
	char get_site_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=1";
	char get_buffer[1024];

	dive_start();
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));

	if (data[4])
		utf8_string(data[4], &cur_dive->notes);

	/* data[5] should have information on Units used, but I cannot
	 * parse it at all based on the sample log I have received. The
	 * temperatures in the samples are all Imperial, so let's go by
	 * that.
	 */

	metric = 0;

	/* Cobalt stores the pressures, not the depth */
	if (data[6])
		cur_dive->dc.maxdepth.mm = atoi(data[6]);

	if (data[7])
		cur_dive->dc.duration.seconds = atoi(data[7]);

	if (data[8])
		cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start();
	dc_settings_start();
	if (data[9]) {
		utf8_string(data[9], &cur_settings.dc.serial_nr);
		cur_settings.dc.deviceid = atoi(data[9]);
		cur_settings.dc.model = strdup("Cobalt import");
	}

	dc_settings_end();
	settings_end();

	if (data[9]) {
		cur_dive->dc.deviceid = atoi(data[9]);
		cur_dive->dc.model = strdup("Cobalt import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_cylinders, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_cylinders failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_buddy_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_buddies, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_buddies failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_visibility_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_visibility, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_visibility failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_location_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_location, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_location failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_site_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_location, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_location (site) failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_profile_sample, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_profile_sample failed.\n");
		return 1;
	}

	dive_end();

	return SQLITE_OK;
}


int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
			    struct dive_table *table)
{
	(void) buffer;
	(void) size;

	int retval;
	char *err = NULL;
	target_table = table;

	char get_dives[] = "select Id,strftime('%s',DiveStartTime),LocationId,'buddy','notes',Units,(MaxDepthPressure*10000/SurfacePressure)-10000,DiveMinutes,SurfacePressure,SerialNumber,'model' from Dive where IsViewDeleted = 0";

	retval = sqlite3_exec(handle, get_dives, &cobalt_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}

extern int divinglog_cylinder(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

	short dbl = 1;
	//char get_cylinder_template[] = "select TankID,TankSize,PresS,PresE,PresW,O2,He,DblTank from Tank where LogID = %d";

	/*
	 * Divinglog might have more cylinders than what we support. So
	 * better to ignore those.
	 */

	if (cur_cylinder_index >= MAX_CYLINDERS)
		return 0;

	if (data[7] && atoi(data[7]) > 0)
		dbl = 2;

	cylinder_start();

	/*
	 * Assuming that we have to double the cylinder size, if double
	 * is set
	 */

	if (data[1] && atoi(data[1]) > 0)
		cur_dive->cylinder[cur_cylinder_index].type.size.mliter = atol(data[1]) * 1000 * dbl;

	if (data[2] && atoi(data[2]) > 0)
		cur_dive->cylinder[cur_cylinder_index].start.mbar = atol(data[2]) * 1000;
	if (data[3] && atoi(data[3]) > 0)
		cur_dive->cylinder[cur_cylinder_index].end.mbar = atol(data[3]) * 1000;
	if (data[4] && atoi(data[4]) > 0)
		cur_dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = atol(data[4]) * 1000;
	if (data[5] && atoi(data[5]) > 0)
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = atol(data[5]) * 10;
	if (data[6] && atoi(data[6]) > 0)
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = atol(data[6]) * 10;

	cylinder_end();

	return 0;
}

extern int divinglog_profile(void *handle, int columns, char **data, char **column)
{
	(void) handle;
	(void) columns;
	(void) column;

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
		sample_start();

		cur_sample->time.seconds = time;
		cur_sample->in_deco = ptr1[5] - '0' ? true : false;
		cur_sample->depth.mm = atoi_n(ptr1, 5) * 10;

		if (len2 >= 11) {
			int temp = atoi_n(ptr2, 3);
			int pressure = atoi_n(ptr2+3, 4);
			int tank = atoi_n(ptr2+7, 1);
			int rbt = atoi_n(ptr2+8, 3) * 60;

			cur_sample->temperature.mkelvin = C_to_mkelvin(temp / 10.0f);
			cur_sample->pressure[0].mbar = pressure * 100;
			cur_sample->rbt.seconds = rbt;
			if (oldcyl != tank) {
				struct gasmix *mix = &cur_dive->cylinder[tank].gasmix;
				int o2 = get_o2(mix);
				int he = get_he(mix);

				event_start();
				cur_event.time.seconds = time;
				strcpy(cur_event.name, "gaschange");

				o2 = (o2 + 5) / 10;
				he = (he + 5) / 10;
				cur_event.value = o2 + (he << 16);

				event_end();
				oldcyl = tank;
			}

			ptr2 += 11; len2 -= 11;
		}

		if (len3 >= 14) {
			cur_sample->heartbeat = atoi_n(ptr3+8, 3);
			ptr3 += 14; len3 -= 14;
		}

		if (len4 >= 9) {
			/*
			 * Following value is NDL when not in deco, and
			 * either 0 or TTS when in deco.
			 */
			int val = atoi_n(ptr4, 3);
			if (cur_sample->in_deco) {
				cur_sample->ndl.seconds = 0;
				if (val)
					cur_sample->tts.seconds = val * 60;
			} else {
				cur_sample->ndl.seconds = val * 60;
			}
			cur_sample->stoptime.seconds = atoi_n(ptr4+3, 3) * 60;
			cur_sample->stopdepth.mm = atoi_n(ptr4+6, 3) * 1000;
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
			(void) otu; // we seem to not store this? Do we understand its format?
			int cns = atoi_n(ptr5 + 13, 4);
			int setpoint = atoi_n(ptr5 + 17, 2);

			if (ppo2_1 > 0)
				cur_sample->o2sensor[0].mbar = ppo2_1 * 100;
			if (ppo2_2 > 0)
				cur_sample->o2sensor[1].mbar = ppo2_2 * 100;
			if (ppo2_3 > 0)
				cur_sample->o2sensor[2].mbar = ppo2_3 * 100;
			if (cns > 0)
				cur_sample->cns = lrintf(cns / 10.0f);
			if (setpoint > 0)
				cur_sample->setpoint.mbar = setpoint * 100;
			ptr5 += 19; len5 -= 19;
		}

		/*
		 * Count the number of o2 sensors
		 */

		if (!cur_dive->dc.no_o2sensors && (cur_sample->o2sensor[0].mbar || cur_sample->o2sensor[0].mbar || cur_sample->o2sensor[0].mbar)) {
			cur_dive->dc.no_o2sensors = cur_sample->o2sensor[0].mbar ? 1 : 0 +
				 cur_sample->o2sensor[1].mbar ? 1 : 0 +
				 cur_sample->o2sensor[2].mbar ? 1 : 0;
		}

		sample_end();

		/* Remaining bottom time warning */
		if (ptr1[6] - '0') {
			event_start();
			cur_event.time.seconds = time;
			strcpy(cur_event.name, "rbt");
			event_end();
		}

		/* Ascent warning */
		if (ptr1[7] - '0') {
			event_start();
			cur_event.time.seconds = time;
			strcpy(cur_event.name, "ascent");
			event_end();
		}

		/* Deco stop ignored */
		if (ptr1[8] - '0') {
			event_start();
			cur_event.time.seconds = time;
			strcpy(cur_event.name, "violation");
			event_end();
		}

		/* Workload warning */
		if (ptr1[9] - '0') {
			event_start();
			cur_event.time.seconds = time;
			strcpy(cur_event.name, "workload");
			event_end();
		}

		ptr1 += 12; len1 -= 12;
		time += sinterval;
	}

	return 0;
}

extern int divinglog_dive(void *param, int columns, char **data, char **column)
{
	(void) columns;
	(void) column;

	int retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	char *err = NULL;
	char get_profile_template[] = "select ProfileInt,Profile,Profile2,Profile3,Profile4,Profile5 from Logbook where ID = %d";
	char get_cylinder0_template[] = "select 0,TankSize,PresS,PresE,PresW,O2,He,DblTank from Logbook where ID = %d";
	char get_cylinder_template[] = "select TankID,TankSize,PresS,PresE,PresW,O2,He,DblTank from Tank where LogID = %d order by TankID";
	char get_buffer[1024];

	dive_start();
	diveid = atoi(data[13]);
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));

	if (data[2])
		cur_dive->dive_site_uuid = find_or_create_dive_site_with_name(data[2], cur_dive->when);

	if (data[3])
		utf8_string(data[3], &cur_dive->buddy);

	if (data[4])
		utf8_string(data[4], &cur_dive->notes);

	if (data[5])
		cur_dive->dc.maxdepth.mm = lrint(strtod_flags(data[5], NULL, 0) * 1000);

	if (data[6])
		cur_dive->dc.duration.seconds = atoi(data[6]) * 60;

	if (data[7])
		utf8_string(data[7], &cur_dive->divemaster);

	if (data[8])
		cur_dive->airtemp.mkelvin = C_to_mkelvin(atol(data[8]));

	if (data[9])
		cur_dive->watertemp.mkelvin = C_to_mkelvin(atol(data[9]));

	if (data[10]) {
		cur_dive->weightsystem[0].weight.grams = atol(data[10]) * 1000;
		cur_dive->weightsystem[0].description = strdup(translate("gettextFromC", "unknown"));
	}

	if (data[11])
		cur_dive->suit = strdup(data[11]);

	/* Divinglog has following visibility options: good, medium, bad */
	if (data[14]) {
		switch(data[14][0]) {
		case '0':
			break;
		case '1':
			cur_dive->visibility = 5;
			break;
		case '2':
			cur_dive->visibility = 3;
			break;
		case '3':
			cur_dive->visibility = 1;
			break;
		default:
			break;
		}
	}

	settings_start();
	dc_settings_start();

	if (data[12]) {
		cur_dive->dc.model = strdup(data[12]);
	} else {
		cur_settings.dc.model = strdup("Divinglog import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder0_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_cylinder, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query divinglog_cylinder0 failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_cylinder, 0, &err);
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
			cur_dive->dc.divemode = PSCR;
			break;
		case '2':
			cur_dive->dc.divemode = CCR;
			break;
		}
	}

	dc_settings_end();
	settings_end();

	if (data[12]) {
		cur_dive->dc.model = strdup(data[12]);
	} else {
		cur_dive->dc.model = strdup("Divinglog import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, diveid);
	retval = sqlite3_exec(handle, get_buffer, &divinglog_profile, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query divinglog_profile failed.\n");
		return 1;
	}

	dive_end();

	return SQLITE_OK;
}


int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
			    struct dive_table *table)
{
	(void) buffer;
	(void) size;

	int retval;
	char *err = NULL;
	target_table = table;

	char get_dives[] = "select Number,strftime('%s',Divedate || ' ' || ifnull(Entrytime,'00:00')),Country || ' - ' || City || ' - ' || Place,Buddy,Comments,Depth,Divetime,Divemaster,Airtemp,Watertemp,Weight,Divesuit,Computer,ID,Visibility,SupplyType from Logbook where UUID not in (select UUID from DeletedRecords)";

	retval = sqlite3_exec(handle, get_dives, &divinglog_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}
