#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H

/* libdivecomputer */
#include <libdivecomputer/version.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

/* handling uemis Zurich SDA files */
#include "uemis.h"

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct device_data_t {
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	const char *model;
	unsigned int deviceid, diveid;
	dc_device_t *device;
	dc_context_t *context;
	int preexisting;
	gboolean force_download;
#if USE_GTK_UI
	progressbar_t progress;
	GtkDialog *dialog;
#endif
} device_data_t;

extern GError *do_import(device_data_t *data);

#endif
