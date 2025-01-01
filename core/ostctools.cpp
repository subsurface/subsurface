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
#include "file.h"
#include "format.h"
#include "libdivecomputer.h"

/*
 * OSTCTools stores the raw dive data in heavily padded files, one dive
 * each file. So it's not necessary to iterate once and again on a parsing
 * function. Actually there's only one kind of archive for every DC model.
 */
int ostctools_import(std::string &buffer, struct divelog *log)
{
	if (buffer.size() < 456)
		return report_error("%s", translate("gettextFromC", "Invalid OSTCTools file"));

	// Read dive number from the log
	auto ostcdive = std::make_unique<dive>();
	ostcdive->number = (unsigned char)buffer[258] + ((unsigned char)buffer[259] << 8);


	// Read device's serial number
	unsigned int serial = (unsigned char)buffer[265] + ((unsigned char)buffer[266] << 8);

	// Trim the buffer to the actual dive data
	buffer = buffer.substr(456);
	unsigned int i = 0;
	bool end_marker = false;
	for (auto c: buffer) {
		i++;
		if ((unsigned char)c == 0xFD) {
			if (end_marker)
				break;
			else
				end_marker = true;
		} else {
			end_marker = false;
		}
	}
	if (end_marker)
		buffer = buffer.substr(0, i);

	// Try to determine the dc family based on the header type
	dc_family_t dc_fam;
	if ((unsigned char)buffer[2] == 0x20 || (unsigned char)buffer[2] == 0x21) {
		dc_fam = DC_FAMILY_HW_OSTC;
	} else {
		switch ((unsigned char)buffer[8]) {
		case 0x22:
			dc_fam = DC_FAMILY_HW_FROG;
			break;
		case 0x23:
		case 0x24:
			dc_fam = DC_FAMILY_HW_OSTC3;
			break;
		default:
			return report_error(translate("gettextFromC", "Unknown DC in dive %d"), ostcdive->number);
		}
	}

	// Try to determine the model based on serial number
	int model;
	switch (dc_fam) {
	case DC_FAMILY_HW_OSTC:
		if (serial > 7000)
			model = 3; //2C
		else if (serial > 2048)
			model = 2; //2N
		else if (serial > 300)
			model = 1; //MK2
		else
			model = 0; //OSTC
		break;
	case DC_FAMILY_HW_FROG:
		model = 0;
		break;
	default:
		if (serial > 10000)
			model = 0x12; //Sport
		else
			model = 0x0A; //OSTC3
	}

	// Prepare data to pass to libdivecomputer.
	device_data_t devdata;
	devdata.log = log;
	int ret = prepare_device_descriptor(model, dc_fam, devdata);
	if (ret == 0)
		return report_error(translate("gettextFromC", "Unknown DC in dive %d"), ostcdive->number);
	ostcdive->dcs[0].model = devdata.vendor + " " + devdata.model + " (Imported from OSTCTools)";

	// Parse the dive data
	dc_status_t rc = libdc_buffer_parser(ostcdive.get(), &devdata, (unsigned char *)buffer.data(), buffer.size());
	if (rc != DC_STATUS_SUCCESS)
		return report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), ostcdive->number);

	// Serial number is not part of the header nor the profile, so libdc won't
	// catch it. If Serial is part of the extra_data, and set to zero, replace it.
	ostcdive->dcs[0].serial = std::to_string(serial);

	auto it = find_if(ostcdive->dcs[0].extra_data.begin(), ostcdive->dcs[0].extra_data.end(),
			 [](auto &ed) { return ed.key == "Serial"; });
	if (it != ostcdive->dcs[0].extra_data.end() && it->value == "0")
		it->value = ostcdive->dcs[0].serial.c_str();
	else if (it == ostcdive->dcs[0].extra_data.end())
		add_extra_data(&ostcdive->dcs[0], "Serial", ostcdive->dcs[0].serial);

	log->dives.record_dive(std::move(ostcdive));

	return 1;
}
