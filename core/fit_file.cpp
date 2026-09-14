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

int fit_file_import(const std::string &buffer, struct divelog *log)
{
	int model = 0;

	device_data_t devdata;
	devdata.log = log;
	int ret = prepare_device_descriptor(model, DC_FAMILY_GARMIN, devdata);
	if (ret == 0)
		return report_error("%s", translate("gettextFromC", "Unknown DC"));

	// AI-generated (Claude)
	// FIT imports begin with the generic descriptor so that the Garmin parser
	// can identify the product. Use an exact descriptor only when its reported
	// product ID is known; malformed and unsupported FIT files retain the
	// existing generic fallback.
	dc_parser_t *parser = NULL;
	dc_status_t rc = dc_parser_new2(&parser, devdata.context, devdata.descriptor,
		(const unsigned char *)buffer.data(), buffer.size());
	if (rc == DC_STATUS_SUCCESS) {
		dc_event_devinfo_t devinfo = {};
		if (dc_parser_get_device_info(parser, &devinfo) == DC_STATUS_SUCCESS &&
		    devinfo.model != 0)
			prepare_device_descriptor(devinfo.model, DC_FAMILY_GARMIN, devdata);
		dc_parser_destroy(parser);
	}

	auto d = std::make_unique<dive>();
	d->dcs[0].model = devdata.vendor + " " + devdata.model + " (Imported from file)";

	// Parse the dive data
	rc = libdc_buffer_parser(d.get(), &devdata, (const unsigned char *)buffer.data(), buffer.size());
	if (rc != DC_STATUS_SUCCESS)
		return report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), d->number);

	log->dives.record_dive(std::move(d));

	return 1;
}
