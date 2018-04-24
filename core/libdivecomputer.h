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

#include "dive.h"

#ifdef __cplusplus
extern "C" {
#endif

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct dc_user_device_t
{
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	const char *model;
	unsigned char *fingerprint;
	unsigned int fsize, fdiveid;
	uint32_t libdc_firmware, libdc_serial;
	uint32_t deviceid, diveid;
	dc_device_t *device;
	dc_context_t *context;
	dc_iostream_t *iostream;
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
extern void (*progress_callback)(const char *text);
extern double progress_bar_fraction;
extern char *logfile_name;
extern char *dumpfile_name;

dc_status_t ble_packet_open(dc_iostream_t **iostream, dc_context_t *context, const char* devaddr, void *userdata);
dc_status_t rfcomm_stream_open(dc_iostream_t **iostream, dc_context_t *context, const char* devaddr);
dc_status_t ftdi_open(dc_iostream_t **iostream, dc_context_t *context);

dc_status_t divecomputer_device_open(device_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // LIBDIVECOMPUTER_H
