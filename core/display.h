// SPDX-License-Identifier: GPL-2.0
#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int nr_cylinders;
	int maxtime;
	int meandepth, maxdepth;
	int minpressure, maxpressure;
	int minhr, maxhr;
	int mintemp, maxtemp;
	enum {AIR, NITROX, TRIMIX, FREEDIVING} dive_type;
	double endtempcoord;
	double maxpp;
	bool waypoint_above_ceiling;
	struct plot_data *entry;
	struct plot_pressure_data *pressures; /* cylinders.nr blocks of nr entries. */
};

#define AMB_PERCENTAGE 50.0

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
