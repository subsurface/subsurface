// SPDX-License-Identifier: GPL-2.0

#include "sample.h"

/*
 * Adding a cylinder pressure sample field is not quite as trivial as it
 * perhaps should be.
 *
 * We try to keep the same sensor index for the same sensor, so that even
 * if the dive computer doesn't give pressure information for every sample,
 * we don't move pressure information around between the different sensor
 * indices.
 *
 * The "prepare_sample()" function will always copy the sensor indices
 * from the previous sample, so the indices are pre-populated (but the
 * pressures obviously are not)
 */
void add_sample_pressure(struct sample *sample, int sensor, int mbar)
{
	int idx;

	if (!mbar)
		return;

	/* Do we already have a slot for this sensor */
	for (idx = 0; idx < MAX_SENSORS; idx++) {
		if (sensor != sample->sensor[idx])
			continue;
		sample->pressure[idx].mbar = mbar;
		return;
	}

	/* Pick the first unused index if we couldn't reuse one */
	for (idx = 0; idx < MAX_SENSORS; idx++) {
		if (sample->pressure[idx].mbar)
			continue;
		sample->sensor[idx] = sensor;
		sample->pressure[idx].mbar = mbar;
		return;
	}

	/* We do not have enough slots for the pressure samples. */
	/* Should we warn the user about dropping pressure data? */
}

