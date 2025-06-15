// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "errorhelper.h"
#include "subsurface-string.h"
#include "gettext.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "extradata.h"
#include "format.h"
#include "libdivecomputer.h"

// As supplied by Divesoft
static constexpr std::string_view divesoft_log_version_1 = "DivE";
static constexpr std::string_view divesoft_log_version_2 = "DiVE";

static constexpr unsigned divesoft_factory_test_dive = 0x02;

static constexpr std::string_view divesoft_freedom_serial_prefix = "7044";
static constexpr std::string_view divesoft_freedom_plus_hw_4_serial_prefix = "7273";
static constexpr std::string_view divesoft_freedom_plus_hw_5_serial_prefix = "7185";
static constexpr std::string_view divesoft_liberty_serial_prefix = "7026";
static constexpr std::string_view divesoft_liberty_2021_serial_prefix = "7203";

// From libdivecomputer
static constexpr int divesoft_liberty_model = 10;
static constexpr int divesoft_freedom_model = 19;

int divesoft_import(const std::string &buffer, struct divelog *log)
{
	int model;
	std::string model_name;

	std::string log_version = buffer.substr(0, 4);
	if (log_version == divesoft_log_version_1) {
		model = divesoft_freedom_model;
		model_name = "[unknown model, V1 log]";
	} else if (log_version == divesoft_log_version_2) {
		unsigned dive_type = static_cast<unsigned>(buffer.substr(19, 1)[0]);
		if ((dive_type & 0x07) == divesoft_factory_test_dive) {
			model = divesoft_freedom_model;
			model_name = "[unknown model, factory test dive]";
		} else {
			std::string model_identifier = buffer.substr(52, 4);
			if (model_identifier == divesoft_liberty_serial_prefix || model_identifier == divesoft_liberty_2021_serial_prefix) {
				model = divesoft_liberty_model;
			} else if (model_identifier == divesoft_freedom_serial_prefix || model_identifier == divesoft_freedom_plus_hw_4_serial_prefix || model_identifier == divesoft_freedom_plus_hw_5_serial_prefix) {
				model = divesoft_freedom_model;
			} else {
				report_error(translate("gettextFromC", "Unknown Divesoft model %s, using Freedom as default"), model_identifier.c_str());

				model = divesoft_freedom_model;
				model_name = "[unknown model]";
			}
		}
	} else {
		return report_error("%s", translate("gettextFromC", "Not a known Divesoft log format."));
	}

	device_data_t devdata;
	devdata.log = log;
	int ret = prepare_device_descriptor(model, DC_FAMILY_DIVESOFT_FREEDOM, devdata);
	if (ret == 0)
		return report_error("%s", translate("gettextFromC", "Unknown DC"));

	auto d = std::make_unique<dive>();
	d->dcs[0].model = devdata.vendor + " " + (model_name != "" ? model_name : devdata.model) + " (Imported from file)";

	// Parse the dive data
	dc_status_t rc = libdc_buffer_parser(d.get(), &devdata, (const unsigned char *)buffer.data(), buffer.size());
	if (rc != DC_STATUS_SUCCESS)
		return report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), d->number);

	log->dives.record_dive(std::move(d));

	return 1;
}
