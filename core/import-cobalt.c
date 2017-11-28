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

