#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H

/* libdivecomputer */
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>
#include <libdivecomputer/utils.h>

/* handling uemis Zurich SDA files */
#include "uemis.h"

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct device_data_t {
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	dc_device_t *device;
	progressbar_t progress;
	int preexisting;
} device_data_t;

extern GError *do_import(device_data_t *data);

#endif
