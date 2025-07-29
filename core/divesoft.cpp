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

// From libdivecomputer
static constexpr int divesoft_liberty_model = 10;
static constexpr int divesoft_freedom_model = 19;

int divesoft_import(const std::string &buffer, struct divelog *log)
{
	device_data_t devdata;
	devdata.log = log;
	int ret = prepare_device_descriptor(divesoft_freedom_model, DC_FAMILY_DIVESOFT_FREEDOM, devdata);
	if (ret == 0)
		return report_error("%s", translate("gettextFromC", "Unknown DC"));

	auto d = std::make_unique<dive>();
	d->dcs[0].model = devdata.vendor + " (Imported from file)";

	// Parse the dive data
	dc_status_t rc = libdc_buffer_parser(d.get(), &devdata, (const unsigned char *)buffer.data(), buffer.size());
	if (rc != DC_STATUS_SUCCESS)
		return report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), d->number);

	log->dives.record_dive(std::move(d));

	return 1;
}
