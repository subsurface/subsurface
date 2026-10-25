// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "dive.h"
#include "sample.h"
#include "subsurface-string.h"
#include "parse.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "device.h"
#include "errorhelper.h"
#include "membuffer.h"
#include "gettext.h"
#include "libdivecomputer.h"
#include "sha1.h"

#include <algorithm>
#include <stdlib.h>
#include <zlib.h>

static int shearwater_cylinders(void *param, int, char **data, char **)
{
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	int o2 = lrint(permissive_strtod(data[0], NULL) * 1000);
	int he = lrint(permissive_strtod(data[1], NULL) * 1000);

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

static int shearwater_changes(void *param, int columns, char **data, char **)
{
	struct parser_state *state = (struct parser_state *)param;
	cylinder_t *cyl;

	if (columns != 3) {
		return 1;
	}
	if (!data[0] || !data[1] || !data[2]) {
		return 2;
	}
	int o2 = lrint(permissive_strtod(data[1], NULL) * 1000);
	int he = lrint(permissive_strtod(data[2], NULL) * 1000);

	/* Shearwater allows entering only 99%, not 100%
	 * so assume 99% to be pure oxygen */
	if (o2 == 990 && he == 0)
		o2 = 1000;

	// Find the cylinder index
	auto it = std::find_if(state->cur_dive->cylinders.begin(), state->cur_dive->cylinders.end(),
			      [o2, he](auto &cyl)
			      { return cyl.gasmix.o2.permille == o2 && cyl.gasmix.he.permille == he; });
	if (it == state->cur_dive->cylinders.end()) {
		// Cylinder not found, creating a new one
		cyl = cylinder_start(state);
		cyl->gasmix.o2.permille = o2;
		cyl->gasmix.he.permille = he;
		cylinder_end(state);
		it = std::prev(state->cur_dive->cylinders.end());
	}

	add_gas_switch_event(state->cur_dive.get(), get_dc(state), state->sample_rate ? atoi(data[0]) / state->sample_rate * 10 : atoi(data[0]),
			     it - state->cur_dive->cylinders.begin());
	return 0;
}

static int shearwater_profile_sample(void *param, int, char **data, char **)
{
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
		state->cur_sample->depth.mm = state->metric ? lrint(permissive_strtod(data[1], NULL) * 1000) : feet_to_mm(permissive_strtod(data[1], NULL));
	if (data[2])
		state->cur_sample->temperature.mkelvin = state->metric ? C_to_mkelvin(permissive_strtod(data[2], NULL)) : F_to_mkelvin(permissive_strtod(data[2], NULL));
	if (data[3])
		state->cur_sample->setpoint.mbar = lrint(permissive_strtod(data[3], NULL) * 1000);
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

static int shearwater_ai_profile_sample(void *param, int, char **data, char **)
{
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
		state->cur_sample->depth.mm = state->metric ? lrint(permissive_strtod(data[1], NULL) * 1000) : feet_to_mm(permissive_strtod(data[1], NULL));
	if (data[2])
		state->cur_sample->temperature.mkelvin = state->metric ? C_to_mkelvin(permissive_strtod(data[2], NULL)) : F_to_mkelvin(permissive_strtod(data[2], NULL));
	if (data[3]) {
		state->cur_sample->setpoint.mbar = lrint(permissive_strtod(data[3], NULL) * 1000);
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

static int shearwater_mode(void *param, int, char **data, char **)
{
	struct parser_state *state = (struct parser_state *)param;

	if (data[0])
		state->cur_dive->dcs[0].divemode = atoi(data[0]) == 0 ? CCR : OC;

	return 0;
}

static int shearwater_dive(void *param, int, char **data, char **)
{
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
		add_dive_site(data[2], state->cur_dive.get(), state);
	if (data[3])
		utf8_string_std(data[3], &state->cur_dive->buddy);
	if (data[4])
		utf8_string_std(data[4], &state->cur_dive->notes);

	state->metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		state->cur_dive->dcs[0].maxdepth.mm = state->metric ? lrint(permissive_strtod(data[6], NULL) * 1000) : feet_to_mm(permissive_strtod(data[6], NULL));

	if (data[7])
		state->cur_dive->dcs[0].duration.seconds = atoi(data[7]) * 60;

	if (data[8])
		state->cur_dive->dcs[0].surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[9])
		utf8_string_std(data[9], &state->cur_settings.dc.serial_nr);
	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_settings.dc.model = "Shearwater Petrel/Perdix";
			break;
		case 4:
			state->cur_settings.dc.model = "Shearwater Predator";
			break;
		default:
			state->cur_settings.dc.model = "Shearwater import";
			break;
		}
	}

	state->cur_settings.dc.deviceid = atoi(data[9]);

	dc_settings_end(state);
	settings_end(state);

	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_dive->dcs[0].model = "Shearwater Petrel/Perdix";
			break;
		case 4:
			state->cur_dive->dcs[0].model = "Shearwater Predator";
			break;
		default:
			state->cur_dive->dcs[0].model = "Shearwater import";
			break;
		}
	}

	if (data[11]) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_mode_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_mode, state, NULL);
		if (retval != SQLITE_OK) {
			report_info("Database query shearwater_mode failed.");
			return 1;
		}
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		report_info("Database query shearwater_cylinders failed.");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		report_info("Database query shearwater_changes failed.");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template_ai, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_ai_profile_sample, state, NULL);
	if (retval != SQLITE_OK) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, state, NULL);
		if (retval != SQLITE_OK) {
			report_info("Database query shearwater_profile_sample failed.");
			return 1;
		}
	}

	dive_end(state);

	return SQLITE_OK;
}

static int shearwater_cloud_dive(void *param, int, char **data, char **)
{
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
		add_dive_site(data[2], state->cur_dive.get(), state);
	if (data[3])
		utf8_string_std(data[3], &state->cur_dive->buddy);
	if (data[4])
		utf8_string_std(data[4], &state->cur_dive->notes);

	state->metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		state->cur_dive->dcs[0].maxdepth.mm = state->metric ? lrint(permissive_strtod(data[6], NULL) * 1000) : feet_to_mm(permissive_strtod(data[6], NULL));

	if (data[7])
		state->cur_dive->dcs[0].duration.seconds = atoi(data[7]);

	if (data[8])
		state->cur_dive->dcs[0].surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start(state);
	dc_settings_start(state);
	if (data[9])
		utf8_string_std(data[9], &state->cur_settings.dc.serial_nr);
	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_settings.dc.model = "Shearwater Petrel/Perdix";
			break;
		case 4:
			state->cur_settings.dc.model = "Shearwater Predator";
			break;
		default:
			state->cur_settings.dc.model = "Shearwater import";
			break;
		}
	}

	state->cur_settings.dc.deviceid = atoi(data[9]);

	dc_settings_end(state);
	settings_end(state);

	if (data[10]) {
		switch (atoi(data[10])) {
		case 2:
			state->cur_dive->dcs[0].model = "Shearwater Petrel/Perdix";
			break;
		case 4:
			state->cur_dive->dcs[0].model = "Shearwater Predator";
			break;
		default:
			state->cur_dive->dcs[0].model = "Shearwater import";
			break;
		}
	}

	if (data[11]) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_mode_template, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_mode, state, NULL);
		if (retval != SQLITE_OK) {
			report_info("Database query shearwater_mode failed.");
			return 1;
		}
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, state, NULL);
	if (retval != SQLITE_OK) {
		report_info("Database query shearwater_cylinders failed.");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_first_gas_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		report_info("Database query shearwater_changes failed.");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, state, NULL);
	if (retval != SQLITE_OK) {
		report_info("Database query shearwater_changes failed.");
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template_ai, dive_id, dive_id);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_ai_profile_sample, state, NULL);
	if (retval != SQLITE_OK) {
		snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, dive_id, dive_id);
		retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, state, NULL);
		if (retval != SQLITE_OK) {
			report_info("Database query shearwater_profile_sample failed.");
			return 1;
		}
	}

	dive_end(state);

	return SQLITE_OK;
}

// AI-generated (Claude)
// Newer versions of Shearwater Cloud leave dive_logs and dive_log_records
// empty and keep each dive only as the log the dive computer wrote, in
// log_data.data_bytes_1: a little-endian 32-bit length of the uncompressed
// log, followed by a gzip stream. The uncompressed log is the same data a
// Bluetooth download returns, so libdivecomputer can parse it.
static constexpr uint32_t shearwater_cloud_max_log_size = 16 * 1024 * 1024;
static constexpr size_t shearwater_record_size = 32;
static constexpr unsigned char shearwater_final_record = 0xFF;
static constexpr size_t shearwater_final_serial_offset = 2;
static constexpr size_t shearwater_final_model_offset = 13;
static constexpr size_t shearwater_fingerprint_offset = 12;
static constexpr size_t shearwater_fingerprint_size = 4;
// From libdivecomputer
static constexpr int shearwater_petrel_model = 3;

static std::string shearwater_cloud_uncompress(const unsigned char *blob, int size)
{
	if (!blob || size < 4)
		return std::string();
	uint32_t length = blob[0] | (blob[1] << 8) | (blob[2] << 16) | ((uint32_t)blob[3] << 24);
	if (length == 0 || length > shearwater_cloud_max_log_size)
		return std::string();

	std::string log(length, '\0');
	z_stream stream = {};
	// 16 + MAX_WBITS: expect a gzip header rather than a zlib one
	if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK)
		return std::string();
	stream.next_in = const_cast<unsigned char *>(blob + 4);
	stream.avail_in = size - 4;
	stream.next_out = reinterpret_cast<unsigned char *>(log.data());
	stream.avail_out = length;
	int ret = inflate(&stream, Z_FINISH);
	uLong total = stream.total_out;
	inflateEnd(&stream);
	if (ret != Z_STREAM_END || total != length)
		return std::string();
	return log;
}

// The final record carries the serial number and the model number that libdivecomputer uses
static size_t shearwater_final_record_offset(const std::string &log)
{
	for (size_t offset = log.size() / shearwater_record_size * shearwater_record_size; offset >= shearwater_record_size;) {
		offset -= shearwater_record_size;
		if ((unsigned char)log[offset] == shearwater_final_record)
			return offset;
	}
	return std::string::npos;
}

// libdivecomputer only learns the serial number from the dive computer during a
// download, so the parser reports it as 0. Set it the way a download would.
static void shearwater_set_serial(struct divecomputer &dc, const std::string &log, size_t final)
{
	const unsigned char *p = (const unsigned char *)log.data() + final + shearwater_final_serial_offset;
	uint32_t number = ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	char serial[9];
	snprintf(serial, sizeof(serial), "%08x", number);

	dc.serial = serial;
	dc.deviceid = calculate_string_hash(serial);
	auto it = std::find_if(dc.extra_data.begin(), dc.extra_data.end(),
			       [](auto &ed) { return ed.key == "Serial"; });
	if (it != dc.extra_data.end())
		it->value = serial;
}

static int shearwater_cloud_log_data(sqlite3 *handle, struct divelog *log)
{
	// Only the dives that have no dive_logs row, so that databases written by
	// older versions of Shearwater Cloud still import as before
	char get_logs[] = "select d.DiveNumber,d.Location,d.Site,d.Buddy,d.Notes,l.data_bytes_1 FROM dive_details AS d JOIN log_data AS l ON d.DiveId=l.log_id WHERE l.format='sw-pnf' AND d.DiveId NOT IN (select diveId from dive_logs)";
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(handle, get_logs, -1, &stmt, NULL) != SQLITE_OK) {
		report_info("Database query shearwater_cloud_log_data failed.");
		return 1;
	}

	struct parser_state state;
	state.log = log;

	int step;
	while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
		auto text = [stmt](int column) {
			const unsigned char *value = sqlite3_column_text(stmt, column);
			return value ? std::string((const char *)value) : std::string();
		};
		int number = sqlite3_column_int(stmt, 0);

		std::string data = shearwater_cloud_uncompress((const unsigned char *)sqlite3_column_blob(stmt, 5), sqlite3_column_bytes(stmt, 5));
		if (data.size() < shearwater_fingerprint_offset + shearwater_fingerprint_size) {
			report_error(translate("gettextFromC", "Could not read the Shearwater Cloud log of dive %d"), number);
			continue;
		}

		device_data_t devdata;
		devdata.log = log;
		size_t final = shearwater_final_record_offset(data);
		int model = final != std::string::npos ? (unsigned char)data[final + shearwater_final_model_offset] : -1;
		std::string model_name;
		if (model < 0 || (!prepare_device_descriptor(model, DC_FAMILY_SHEARWATER_PETREL, devdata) &&
				  !prepare_device_descriptor(model, DC_FAMILY_SHEARWATER_PREDATOR, devdata))) {
			report_error(translate("gettextFromC", "Unknown Shearwater model %d, using Petrel as default"), model);
			if (!prepare_device_descriptor(shearwater_petrel_model, DC_FAMILY_SHEARWATER_PETREL, devdata)) {
				report_error("%s", translate("gettextFromC", "Unknown DC"));
				continue;
			}
			model_name = "[unknown model]";
		}

		auto d = std::make_unique<dive>();
		d->number = number;
		// Same name as a Bluetooth download, so that both are recognised as the same dive computer
		d->dcs[0].model = devdata.vendor + " " + (!model_name.empty() ? model_name : devdata.model);

		dc_status_t rc = libdc_buffer_parser(d.get(), &devdata, (const unsigned char *)data.data(), data.size());
		if (rc != DC_STATUS_SUCCESS) {
			report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), number);
			// The header parser may already have created a dive site for this dive from its
			// GPS fix. Remove the dive from it, and the site too if no other dive uses it.
			struct dive_site *ds = unregister_dive_from_dive_site(d.get());
			if (ds && ds->dives.empty())
				log->sites.pull(ds);
			continue;
		}

		// libdivecomputer's Shearwater fingerprint, which a Bluetooth download hashes the same way
		d->dcs[0].diveid = SHA1_uint32(data.data() + shearwater_fingerprint_offset, shearwater_fingerprint_size);
		if (final != std::string::npos)
			shearwater_set_serial(d->dcs[0], data, final);

		std::string location = text(1);
		std::string site = text(2);
		std::string name = !location.empty() && !site.empty() ? location + " / " + site : location + site;
		if (!name.empty())
			add_dive_site(name.c_str(), d.get(), &state);
		d->buddy = text(3);
		d->notes = text(4);

		log->dives.record_dive(std::move(d));
	}
	sqlite3_finalize(stmt);

	if (step != SQLITE_DONE) {
		report_info("Database query shearwater_cloud_log_data failed.");
		return 1;
	}
	return 0;
}

int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *, int, struct divelog *log)
{
	int retval;
	struct parser_state state;

	state.log = log;
	state.sql_handle = handle;

	// So far have not seen any sample rate in Shearwater Desktop
	state.sample_rate = 0;

	char get_dives[] = "select l.number,timestamp,location||' / '||site,buddy,notes,imperialUnits,maxDepth,maxTime,startSurfacePressure,computerSerial,computerModel,i.diveId FROM dive_info AS i JOIN dive_logs AS l ON i.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_dive, &state, NULL);

	if (retval != SQLITE_OK) {
		report_info("Database query failed '%s'.", url);
		return 1;
	}

	return 0;
}

int parse_shearwater_cloud_buffer(sqlite3 *handle, const char *url, const char *, int, struct divelog *log)
{
	int retval;
	struct parser_state state;

	state.log = log;
	state.sql_handle = handle;

	char get_dives[] = "select l.number,strftime('%s', DiveDate),location||' / '||site,buddy,notes,imperialUnits,maxDepth,DiveLengthTime,startSurfacePressure,computerSerial,computerModel,d.diveId,l.sampleRateMs / 1000 FROM dive_details AS d JOIN dive_logs AS l ON d.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_cloud_dive, &state, NULL);

	if (retval != SQLITE_OK) {
		report_info("Database query failed '%s'.", url);
		return 1;
	}

	return shearwater_cloud_log_data(handle, log);
}
