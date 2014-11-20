#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H


/* libdivecomputer */
#include <libdivecomputer/version.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

#include "dive.h"

#ifdef __cplusplus
extern "C" {
#endif

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct device_data_t
{
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	const char *model, *serial, *firmware;
	uint32_t deviceid, diveid;
	dc_device_t *device;
	dc_context_t *context;
	struct dive_trip *trip;
	int preexisting;
	bool force_download;
	bool create_new_trip;
	bool libdc_log;
	bool libdc_dump;
	FILE *libdc_logfile;
} device_data_t;

const char *do_libdivecomputer_import(device_data_t *data);
const char *do_uemis_import(device_data_t *data);

extern int import_thread_cancelled;
extern const char *progress_bar_text;
extern double progress_bar_fraction;
extern char *logfile_name;
extern char *dumpfile_name;

#ifdef __cplusplus
}
#endif

#endif // LIBDIVECOMPUTER_H
