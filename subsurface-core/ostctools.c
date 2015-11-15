#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dive.h"
#include "gettext.h"
#include "divelist.h"
#include "libdivecomputer.h"

/*
 * Returns a dc_descriptor_t structure based on dc  model's number and family.
 */

static dc_descriptor_t *ostc_get_data_descriptor(int data_model, dc_family_t data_fam)
{
	dc_descriptor_t *descriptor = NULL, *current = NULL;
	;
	dc_iterator_t *iterator = NULL;
	dc_status_t rc;

	rc = dc_descriptor_iterator(&iterator);
	if (rc != DC_STATUS_SUCCESS) {
		fprintf(stderr, "Error creating the device descriptor iterator.\n");
		return current;
	}
	while ((dc_iterator_next(iterator, &descriptor)) == DC_STATUS_SUCCESS) {
		int desc_model = dc_descriptor_get_model(descriptor);
		dc_family_t desc_fam = dc_descriptor_get_type(descriptor);
		if (data_model == desc_model && data_fam == desc_fam) {
			current = descriptor;
			break;
		}
		dc_descriptor_free(descriptor);
	}
	dc_iterator_free(iterator);
	return current;
}

/*
 * Fills a device_data_t structure with known dc data and a descriptor.
 */
static int ostc_prepare_data(int data_model, dc_family_t dc_fam, device_data_t *dev_data)
{
	dc_descriptor_t *data_descriptor;

	dev_data->device = NULL;
	dev_data->context = NULL;

	data_descriptor = ostc_get_data_descriptor(data_model, dc_fam);
	if (data_descriptor) {
		dev_data->descriptor = data_descriptor;
		dev_data->vendor = copy_string(data_descriptor->vendor);
		dev_data->model = copy_string(data_descriptor->product);
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
void ostctools_import(const char *file, struct dive_table *divetable)
{
	FILE *archive;
	device_data_t *devdata = calloc(1, sizeof(device_data_t));
	dc_family_t dc_fam;
	unsigned char *buffer = calloc(65536, 1), *uc_tmp;
	char *tmp;
	struct dive *ostcdive = alloc_dive();
	dc_status_t rc = 0;
	int model, ret, i = 0;
	unsigned int serial;
	struct extra_data *ptr;

	// Open the archive
	if ((archive = subsurface_fopen(file, "rb")) == NULL) {
		report_error(translate("gettextFromC", "Failed to read '%s'"), file);
		free(ostcdive);
		goto out;
	}

	// Read dive number from the log
	uc_tmp = calloc(2, 1);
	fseek(archive, 258, 0);
	fread(uc_tmp, 1, 2, archive);
	ostcdive->number = uc_tmp[0] + (uc_tmp[1] << 8);
	free(uc_tmp);

	// Read device's serial number
	uc_tmp = calloc(2, 1);
	fseek(archive, 265, 0);
	fread(uc_tmp, 1, 2, archive);
	serial = uc_tmp[0] + (uc_tmp[1] << 8);
	free(uc_tmp);

	// Read dive's raw data, header + profile
	fseek(archive, 456, 0);
	while (!feof(archive)) {
		fread(buffer + i, 1, 1, archive);
		if (buffer[i] == 0xFD && buffer[i - 1] == 0xFD)
			break;
		i++;
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
			dc_fam = DC_FAMILY_HW_OSTC3;
			break;
		default:
			report_error(translate("gettextFromC", "Unknown DC in dive %d"), ostcdive->number);
			free(ostcdive);
			fclose(archive);
			goto out;
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
		free(ostcdive);
		fclose(archive);
		goto out;
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
	mark_divelist_changed(true);
	sort_table(divetable);
	fclose(archive);
out:
	free(devdata);
	free(buffer);
}
