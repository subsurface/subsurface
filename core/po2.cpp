// SPDX-License-Identifier: GPL-2.0
#include "po2.h"
#include "divecomputer.h"

#include <algorithm>

ccr_po2_loop::ccr_po2_loop(const struct divecomputer &dc) : dc(dc)
{
}

bool ccr_po2_loop::uses_sensors() const
{
	return dc.divemode == CCR || (dc.divemode == PSCR && dc.no_o2sensors);
}

/* Sort the o2 pressure values. There are so few that a simple bubble sort
 * will do */
static void sort_o2_pressures(int *sensorn, int np, const pressure_t o2sensor[MAX_O2_SENSORS])
{
	int smallest, position, old;

	for (int i = 0; i < np - 1; i++) {
		position = i;
		smallest = o2sensor[sensorn[i]].mbar;
		for (int j = i + 1; j < np; j++)
			if (o2sensor[sensorn[j]].mbar < smallest) {
				position = j;
				smallest = o2sensor[sensorn[j]].mbar;
			}
		old = sensorn[i];
		sensorn[i] = sensorn[position];
		sensorn[position] = old;
	}
}

/* calculate_ccr_po2(): given the (already carry-forward-completed) sensor
 * readings for one point on a CCR/PSCR dive, calculate the po2 value to use.
 * If there are at least 3 sensors, sensors are voted out until their span is
 * within diff_limit. If none of the sensors has a valid reading, fall back
 * to the caller-supplied value. */
static int calculate_ccr_po2(const pressure_t o2sensor[MAX_O2_SENSORS], int no_o2sensors, pressure_t fallback)
{
	int sump = 0, minp = 0, maxp = 0;
	int sensorn[MAX_O2_SENSORS];
	int i, np = 0;

	for (i = 0; i < no_o2sensors && i < MAX_O2_SENSORS; i++)
		if (o2sensor[i].mbar) { // Valid reading
			sensorn[np++] = i;
			sump += o2sensor[i].mbar;
		}
	if (np == 0)
		return fallback.mbar;
	else if (np == 1)
		return o2sensor[sensorn[0]].mbar;

	maxp = np - 1;
	sort_o2_pressures(sensorn, np, o2sensor);

	// This is the Shearwater voting logic: If there are still at least three sensors and one
	// differs by more than 20% from the closest it is voted out.
	while (maxp - minp > 1) {
		if (o2sensor[sensorn[minp + 1]].mbar - o2sensor[sensorn[minp]].mbar >
		    sump / (maxp - minp + 1) / 5) {
			sump -= o2sensor[sensorn[minp]].mbar;
			++minp;
			continue;
		}
		if (o2sensor[sensorn[maxp]].mbar - o2sensor[sensorn[maxp - 1]].mbar >
		    sump / (maxp - minp + 1) / 5) {
			sump -= o2sensor[sensorn[maxp]].mbar;
			--maxp;
			continue;
		}
		break;
	}

	return sump / (maxp - minp + 1);
}

pressure_t ccr_po2_loop::next(pressure_t o2sensor[MAX_O2_SENSORS], pressure_t fallback, pressure_t ambient)
{
	// Carry forward the last known reading for sensors that did not report
	// a value for this point. last_sensor[] starts out zero-initialised, so
	// this also does the right thing for the very first call: a sensor that
	// has never reported yet is carried forward as zero, same as before any
	// carry-forward state existed.
	for (int j = 0; j < dc.no_o2sensors && j < MAX_O2_SENSORS; j++) {
		if (o2sensor[j].mbar)
			last_sensor[j] = o2sensor[j];
		else
			o2sensor[j] = last_sensor[j];
	}

	pressure_t po2;
	po2.mbar = calculate_ccr_po2(o2sensor, dc.no_o2sensors, fallback);
	po2.mbar = std::min(po2.mbar, ambient.mbar);
	return po2;
}
