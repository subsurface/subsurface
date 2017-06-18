// SPDX-License-Identifier: GPL-2.0
#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H


/* libdivecomputer */

#ifdef DC_VERSION /* prevent a warning with wingdi.h */
#undef DC_VERSION
#endif
#include <libdivecomputer/version.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>
#ifdef SSRF_CUSTOM_IO
#include <libdivecomputer/custom_io.h>
#endif

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
dc_descriptor_t *get_descriptor(dc_family_t type, unsigned int model);

extern int import_thread_cancelled;
extern const char *progress_bar_text;
extern double progress_bar_fraction;
extern char *logfile_name;
extern char *dumpfile_name;

#if SSRF_CUSTOM_IO
// WTF. this symbol never shows up at link time
//extern dc_custom_io_t qt_serial_ops;
// Thats why I've worked around it with a stupid helper returning it.
dc_custom_io_t* get_qt_serial_ops();
extern dc_custom_io_t serial_ftdi_ops;
#endif

#ifdef __cplusplus
}
#endif

#endif // LIBDIVECOMPUTER_H
