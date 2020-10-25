// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "errorhelper.h"
#include "ssrf.h"
#include "subsurface-string.h"
#include "gettext.h"
#include "dive.h"
#include "divelist.h"
#include "extradata.h"
#include "file.h"
#include "libdivecomputer.h"

/*
 * Fills a device_data_t structure with known dc data and a descriptor.
 */
static int ostc_prepare_data(int data_model, dc_family_t dc_fam, device_data_t *dev_data)
{
	dc_descriptor_t *data_descriptor;

	dev_data->device = NULL;
	dev_data->context = NULL;

	data_descriptor = get_descriptor(dc_fam, data_model);
	if (data_descriptor) {
		dev_data->descriptor = data_descriptor;
		dev_data->vendor = copy_string(dc_descriptor_get_vendor(data_descriptor));
		dev_data->model = copy_string(dc_descriptor_get_product(data_descriptor));
	} else {
		return 0;
	}
	return 1;
}

/*
 * OSTCTools stores the raw dive data in heavily padded files, one dive
 * each file. So it's not necessary to iterate once and again on a parsing
 * function. Actually there's only one kind of archive for every DC model.
 */
void ostctools_import(const char *file, struct dive_table *divetable, struct trip_table *trips, struct dive_site_table *sites)
{
	UNUSED(trips);
	UNUSED(sites);
	FILE *archive;
	device_data_t *devdata = calloc(1, sizeof(device_data_t));
	dc_family_t dc_fam;
	unsigned char *buffer = calloc(65536, 1);
	unsigned char uc_tmp[2];
	char *tmp;
	struct dive *ostcdive = alloc_dive();
	dc_status_t rc = 0;
	int model, ret, i = 0, c;
	unsigned int serial;
	struct extra_data *ptr;
	const char *failed_to_read_msg = translate("gettextFromC", "Failed to read '%s'");

	// Open the archive
	if ((archive = subsurface_fopen(file, "rb")) == NULL) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto out;
	}

	// Read dive number from the log
	if (fseek(archive, 258, 0) == -1) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}
	if (fread(uc_tmp, 1, 2, archive) != 2) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}
	ostcdive->number = uc_tmp[0] + (uc_tmp[1] << 8);

	// Read device's serial number
	if (fseek(archive, 265, 0) == -1) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}
	if (fread(uc_tmp, 1, 2, archive) != 2) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}
	serial = uc_tmp[0] + (uc_tmp[1] << 8);

	// Read dive's raw data, header + profile
	if (fseek(archive, 456, 0) == -1) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}
	while ((c = getc(archive)) != EOF) {
		buffer[i] = c;
		if (buffer[i] == 0xFD && buffer[i - 1] == 0xFD)
			break;
		i++;
	}
	if (ferror(archive)) {
		report_error(failed_to_read_msg, file);
		free_dive(ostcdive);
		goto close_out;
	}

	// Try to determine the dc family based on the header type
	if (buffer[2] == 0x20 || buffer[2] == 0x21) {
		dc_fam = DC_FAMILY_HW_OSTC;
	} else {
		switch (buffer[8]) {
		case 0x22:
			dc_fam = DC_FAMILY_HW_FROG;
			break;
		case 0x23:
		case 0x24:
			dc_fam = DC_FAMILY_HW_OSTC3;
			break;
		default:
			report_error(translate("gettextFromC", "Unknown DC in dive %d"), ostcdive->number);
			free_dive(ostcdive);
			goto close_out;
		}
	}

	// Try to determine the model based on serial number
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
	ret = ostc_prepare_data(model, dc_fam, devdata);
	if (ret == 0) {
		report_error(translate("gettextFromC", "Unknown DC in dive %d"), ostcdive->number);
		free_dive(ostcdive);
		goto close_out;
	}
	tmp = calloc(strlen(devdata->vendor) + strlen(devdata->model) + 28, 1);
	sprintf(tmp, "%s %s (Imported from OSTCTools)", devdata->vendor, devdata->model);
	ostcdive->dc.model = copy_string(tmp);
	free(tmp);

	// Parse the dive data
	rc = libdc_buffer_parser(ostcdive, devdata, buffer, i + 1);
	if (rc != DC_STATUS_SUCCESS)
		report_error(translate("gettextFromC", "Error - %s - parsing dive %d"), errmsg(rc), ostcdive->number);

	// Serial number is not part of the header nor the profile, so libdc won't
	// catch it. If Serial is part of the extra_data, and set to zero, remove
	// it from the list and add again.
	tmp = calloc(12, 1);
	sprintf(tmp, "%d", serial);
	ostcdive->dc.serial = copy_string(tmp);
	free(tmp);

	if (ostcdive->dc.extra_data) {
		ptr = ostcdive->dc.extra_data;
		while (strcmp(ptr->key, "Serial"))
			ptr = ptr->next;
		if (!strcmp(ptr->value, "0")) {
			add_extra_data(&ostcdive->dc, "Serial", ostcdive->dc.serial);
			*ptr = *(ptr)->next;
		}
	} else {
		add_extra_data(&ostcdive->dc, "Serial", ostcdive->dc.serial);
	}
	record_dive_to_table(ostcdive, divetable);
	sort_dive_table(divetable);

close_out:
	fclose(archive);
out:
	free(devdata);
	free(buffer);
}
