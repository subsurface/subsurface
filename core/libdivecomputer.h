// SPDX-License-Identifier: GPL-2.0
#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H

#include <stdint.h>
#include <stdio.h>

/* libdivecomputer */

#ifdef DC_VERSION /* prevent a warning with wingdi.h */
#undef DC_VERSION
#endif
#include <libdivecomputer/version.h>
#include <libdivecomputer/device.h>
#include <libdivecomputer/parser.h>

// Even if we have an old libdivecomputer, Uemis uses this
#ifndef DC_TRANSPORT_USBSTORAGE
#define DC_TRANSPORT_USBSTORAGE (1 << 6)
#define dc_usb_storage_open(stream, context, devname) (DC_STATUS_UNSUPPORTED)
#endif

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

struct dive;
struct dive_computer;
struct devices;

typedef struct {
	dc_descriptor_t *descriptor;
	const char *vendor, *product, *devname;
	const char *model, *btname;
	unsigned char *fingerprint;
	unsigned int fsize, fdiveid;
	uint32_t libdc_firmware;
	uint32_t deviceid, diveid;
	dc_device_t *device;
	dc_context_t *context;
	dc_iostream_t *iostream;
	bool force_download;
	bool libdc_log;
	bool libdc_dump;
	bool bluetooth_mode;
	FILE *libdc_logfile;
	struct dive_table *download_table;
	struct dive_site_table *sites;
	struct device_table *devices;
	void *androidUsbDeviceDescriptor;
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
dc_status_t serial_usb_android_open(dc_iostream_t **iostream, dc_context_t *context, void *androidUsbDevice);

dc_status_t divecomputer_device_open(device_data_t *data);

unsigned int get_supported_transports(device_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // LIBDIVECOMPUTER_H
