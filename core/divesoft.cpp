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

int divesoft_import(const std::unique_ptr<std::vector<unsigned char>> &buffer, struct divelog *log)
{
	int model = 0;
	if (strncmp((char *)(buffer->data() + 52), "7026", 4) == 0)
		model = 10;
	else if (strncmp((char *)(buffer->data() + 52), "7044", 4) == 0 || strncmp((char *)(buffer->data() + 52), "7273", 4) == 0)
		model = 19;
	device_data_t devdata;
	int ret = prepare_device_descriptor(model, DC_FAMILY_DIVESOFT_FREEDOM, devdata);
	if (ret == 0)
		return report_error("%s", translate("gettextFromC", "Unknown DC"));

	auto d = std::make_unique<dive>();
	d->dcs[0].model = devdata.vendor + " " + devdata.model + " (Imported from file)";

	// Parse the dive data
	dc_status_t rc = libdc_buffer_parser(d.get(), &devdata, buffer->data(), buffer->size());
	if (rc != DC_STATUS_SUCCESS)
		return report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), d->number);

	log->dives.record_dive(std::move(d));

	return 1;
}
