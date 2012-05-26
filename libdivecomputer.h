#ifndef LIBDIVECOMPUTER_H
#define LIBDIVECOMPUTER_H

/* libdivecomputer */
#include <device.h>
#include <suunto.h>
#include <reefnet.h>
#include <uwatec.h>
#include <oceanic.h>
#include <mares.h>
#include <hw.h>
#include <cressi.h>
#include <zeagle.h>
#include <atomics.h>
#include <utils.h>

/* handling uemis Zurich SDA files */
#include "uemis.h"

/* don't forget to include the UI toolkit specific display-XXX.h first
   to get the definition of progressbar_t */
typedef struct device_data_t {
	device_type_t type;
	const char *name, *devname;
	progressbar_t progress;
	device_devinfo_t devinfo;
	device_clock_t clock;
	int preexisting;
} device_data_t;

struct device_list {
	const char *name;
	device_type_t type;
};

extern struct device_list device_list[];
extern GError *do_import(device_data_t *data);

#endif
