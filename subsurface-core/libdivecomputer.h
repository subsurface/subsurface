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

struct dc_descriptor_t {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct device_data_t
{
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	const char *model;
	uint32_t libdc_firmware, libdc_serial;
	uint32_t deviceid, diveid;
	dc_device_t *device;
	dc_context_t *context;
	struct dive_trip *trip;
	int preexisting;
	bool force_download;
	bool create_new_trip;
	bool libdc_log;
	bool libdc_dump;
	bool bluetooth_mode;
	FILE *libdc_logfile;
	struct dive_table *download_table;
} device_data_t;

const char *errmsg (dc_status_t rc);
const char *do_libdivecomputer_import(device_data_t *data);
const char *do_uemis_import(device_data_t *data);
dc_status_t libdc_buffer_parser(struct dive *dive, device_data_t *data, unsigned char *buffer, int size);
void logfunc(dc_context_t *context, dc_loglevel_t loglevel, const char *file, unsigned int line, const char *function, const char *msg, void *userdata);

extern int import_thread_cancelled;
extern const char *progress_bar_text;
extern double progress_bar_fraction;
extern char *logfile_name;
extern char *dumpfile_name;

#if SSRF_CUSTOM_SERIAL
extern dc_status_t dc_serial_qt_open(dc_serial_t **out, dc_context_t *context, const char *devaddr);
extern dc_status_t dc_serial_ftdi_open(dc_serial_t **out, dc_context_t *context);
#endif

#ifdef __cplusplus
}
#endif

#endif // LIBDIVECOMPUTER_H
