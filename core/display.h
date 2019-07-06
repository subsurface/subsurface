// SPDX-License-Identifier: GPL-2.0
#ifndef DISPLAY_H
#define DISPLAY_H

#include "libdivecomputer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int maxtime;
	int meandepth, maxdepth;
	int minpressure, maxpressure;
	int minhr, maxhr;
	int mintemp, maxtemp;
	enum {AIR, NITROX, TRIMIX, FREEDIVING} dive_type;
	double endtempcoord;
	double maxpp;
	struct plot_data *entry;
};

extern struct divecomputer *select_dc(struct dive *);

extern unsigned int dc_number;

extern unsigned int amount_selected;

extern int is_default_dive_computer_device(const char *);
extern int is_default_dive_computer(const char *, const char *);

typedef void (*device_callback_t)(const char *name, void *userdata);

int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport);

#define AMB_PERCENTAGE 50.0

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
