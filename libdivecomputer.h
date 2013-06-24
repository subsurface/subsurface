#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H


#ifdef __cplusplus
extern "C" {
#endif

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
} device_data_t;

const char *do_libdivecomputer_import(device_data_t *data);
char *do_uemis_import(const char *mountpath, short force_download);

extern int import_thread_cancelled;
extern const char *progress_bar_text;
extern double progress_bar_fraction;

#ifdef __cplusplus
}
#endif

#endif
