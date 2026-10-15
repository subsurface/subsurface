// SPDX-License-Identifier: GPL-2.0
#ifndef PO2_H
#define PO2_H

#include "sample.h" // for MAX_O2_SENSORS
#include "units.h"

struct divecomputer;

/* Resolve the PO2 to use for successive points of a CCR/PSCR dive, the way
 * the dive computer's own sensor voting does (last-known-reading carry
 * forward, Shearwater-style voting among the sensors, clamped to ambient
 * pressure). Call strictly in increasing time order: unreported (zero)
 * sensor readings are carried forward from the previous call. */
class ccr_po2_loop {
	const struct divecomputer &dc;
	pressure_t last_sensor[MAX_O2_SENSORS] = {};
public:
	explicit ccr_po2_loop(const struct divecomputer &dc);

	// True when this dc's samples carry sensor readings that override the
	// commanded setpoint: dc.divemode == CCR, or PSCR with no_o2sensors > 0.
	// When this is false, callers should keep using their own divemode-
	// specific PO2 handling (OC: none; sensor-less PSCR: pscr_o2()).
	bool uses_sensors() const;

	// o2sensor[] is updated in place with carried-forward readings (the
	// profile needs those for its per-sensor traces). fallback is the PO2
	// to use when no sensor reading is available at all; the result is
	// clamped to ambient.
	pressure_t next(pressure_t o2sensor[MAX_O2_SENSORS], pressure_t fallback, pressure_t ambient);
};

#endif
