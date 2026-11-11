// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef TANKGASGUESS_H
#define TANKGASGUESS_H

#include <vector>

// One sample of a dive, as far as guessing the gas mix of each tank goes.
struct tank_gas_sample {
	int gasmix = -1;		// index of the gas mix in use, -1 if not known
	std::vector<int> pressure;	// pressure of each tank in mbar, 0 for no reading
};

// The least a tank must lose on a gas mix to be linked to it, in mbar.
static constexpr int tank_gas_guess_minimum_use = 20000;

std::vector<int> guess_tank_gasmixes(const std::vector<tank_gas_sample> &samples,
				     const std::vector<bool> &sidemount, unsigned int ngases);

#endif // TANKGASGUESS_H
