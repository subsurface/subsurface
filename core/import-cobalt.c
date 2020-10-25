// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include "dive.h"
#include "divesite.h"
#include "gas.h"
#include "parse.h"
#include "sample.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "gettext.h"

static int cobalt_profile_sample(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	sample_start(state);
	if (data[0])
		state->cur_sample->time.seconds = atoi(data[0]);
	if (data[1])
		state->cur_sample->depth.mm = atoi(data[1]);
	if (data[2])
		state->cur_sample->temperature.mkelvin = state->metric ? C_to_mkelvin(strtod_flags(data[2], NULL, 0)) : F_to_mkelvin(strtod_flags(data[2], NULL, 0));
	sample_end(state);

	return 0;
}


static int cobalt_cylinders(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	cyl = cylinder_start(state);
	if (data[0])
		cyl->gasmix.o2.permille = atoi(data[0]) * 10;
	if (data[1])
		cyl->gasmix.he.permille = atoi(data[1]) * 10;
	if (data[2])
		cyl->start.mbar = psi_to_mbar(atoi(data[2]));
	if (data[3])
		cyl->end.mbar = psi_to_mbar(atoi(data[3]));
	if (data[4])
		cyl->type.size.mliter = atoi(data[4]) * 100;
	if (data[5])
		cyl->gas_used.mliter = atoi(data[5]) * 1000;
	cylinder_end(state);

	return 0;
}

static int cobalt_buddies(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	struct parser_state *state = (struct parser_state *)param;

	if (data[0])
		utf8_string(data[0], &state->cur_dive->buddy);

	return 0;
}

/*
 * We still need to figure out how to map free text visibility to
 * Subsurface star rating.
 */

static int cobalt_visibility(void *param, int columns, char **data, char **column)
{
	UNUSED(param);
	UNUSED(columns);
	UNUSED(column);
	UNUSED(data);
	return 0;
}

static int cobalt_location(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);
	char **location = (char **)param;
	*location = data[0] ? strdup(data[0]) : NULL;
	return 0;
}


static int cobalt_dive(void *param, int columns, char **data, char **column)
{
	UNUSED(columns);
	UNUSED(column);

	int retval = 0;
	struct parser_state *state = (struct parser_state *)param;
	sqlite3 *handle = state->sql_handle;
	char *location, *location_site;
	char get_profile_template[] = "select runtime*60,(DepthPressure*10000/SurfacePressure)-10000,p.Temperature from Dive AS d JOIN TrackPoints AS p ON d.Id=p.DiveId where d.Id=%d";
	char get_cylinder_template[] = "select FO2,FHe,StartingPressure,EndingPressure,TankSize,TankPressure,TotalConsumption from GasMixes where DiveID=%d and StartingPressure>0 and EndingPressure > 0 group by FO2,FHe";
	char get_buddy_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=4";
	char get_visibility_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=3";
	char get_location_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=0";
	char get_site_template[] = "select l.Data from Items AS i, List AS l ON i.Value1=l.Id where i.DiveId=%d and l.Type=1";
	char get_buffer[1024];

	dive_start(state);
	state->cur_dive->number = atoi(data[0]);

	state->cur_dive->when = (time_t)(atol(data[1]));

	if (data[4])
		utf8_string(data[4], &state->cur_dive->notes);

	/* data[5] should have information on Units used, but I cannot
	 * parse it at all based on the sample log I have received. The
	 * temperatures in the samples are all Imperial, so let's go by
	 * that.
	 */

	state->metric = 0;

	/* Cobalt stores the pressures, not the depth */
	if (data[6])
		state->cur_dive->dc.maxdepth.mm = atoi(data[6]);

	if (data[7])
		state->cur_dive->dc.duration.seconds = atoi(data[7]);

	if (data[8])
		state->cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[9]) {
		utf8_string(data[9], &state->cur_settings.dc.serial_nr);
		state->cur_settings.dc.deviceid = atoi(data[9]);
		state->cur_settings.dc.model = strdup("Cobalt import");
	}

	dc_settings_end(state);
	settings_end(state);

	if (data[9]) {
		state->cur_dive->dc.deviceid = atoi(data[9]);
		state->cur_dive->dc.model = strdup("Cobalt import");
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_cylinders failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_buddy_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_buddies, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_buddies failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_visibility_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_visibility, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_visibility failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_location_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_location, &location, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_location failed.\n");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_site_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_location, &location_site, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_location (site) failed.\n");
		return 1;
	}

	if (location && location_site) {
		char *tmp = malloc(strlen(location) + strlen(location_site) + 4);
		if (!tmp) {
			free(location);
			free(location_site);
			return 1;
		}
		sprintf(tmp, "%s / %s", location, location_site);
		add_dive_to_dive_site(state->cur_dive, find_or_create_dive_site_with_name(tmp, state->sites));
		free(tmp);
	}
	free(location);
	free(location_site);

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, state->cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &cobalt_profile_sample, state, NULL);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", "Database query cobalt_profile_sample failed.\n");
		return 1;
	}

	dive_end(state);

	return SQLITE_OK;
}


int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
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

	char get_dives[] = "select Id,strftime('%s',DiveStartTime),LocationId,'buddy','notes',Units,(MaxDepthPressure*10000/SurfacePressure)-10000,DiveMinutes,SurfacePressure,SerialNumber,'model' from Dive where IsViewDeleted = 0";

	retval = sqlite3_exec(handle, get_dives, &cobalt_dive, &state, NULL);
	free_parser_state(&state);

	if (retval != SQLITE_OK) {
		fprintf(stderr, "Database query failed '%s'.\n", url);
		return 1;
	}

	return 0;
}
