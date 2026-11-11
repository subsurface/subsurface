// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "tankgasguess.h"

#include <algorithm>
#include <utility>

/**
 * @brief guess_tank_gasmixes guesses which gas mix each tank was breathed on, from
 * how much its pressure dropped while each gas mix was in use
 * Each drop from one reading of a tank to the next counts for the gas mix in use,
 * if that gas mix was in use from one reading to the next, including the samples
 * without a reading in between. A rise takes back the most recent drops, as much as
 * it makes up for: when a valve that was closed is opened, the reading jumps back
 * up from the pressure left in the regulator. A stage whose valve stays closed until
 * the gas switch reads its regulator until then, which drops as it bleeds down or
 * cools, and a valve drill does the same to a tank that is being breathed.
 * If exactly two tanks are flagged as sidemount, they are taken to be one pair
 * breathed alternately on the same gas mix, and count together, unless either of
 * them on its own links to a different gas mix than the two together, or lost
 * more on another gas mix than on the one the two together link to: Shearwater
 * dive computers flag the tanks whose pressures the diver chose to add up for gas
 * time remaining, which may be on different gas mixes. If more are flagged, which
 * of them make up a pair isn't known, so each counts on its own.
 * A tank is linked to the gas mix it lost most on, if that is at least
 * tank_gas_guess_minimum_use, and at least twice what it lost on any other gas mix.
 * Otherwise, the tank is not linked to a gas mix. The minimum is above the ≈13 bar
 * a 200 bar tank loses as it cools by 20 °C from the air to the water, so that an
 * open stage that is carried but not breathed isn't usually linked to the gas mix
 * in use meanwhile.
 * The guess is wrong for a tank breathed by someone else, e.g. a student whose gas
 * an instructor monitors, as the dive computer only knows the instructor's gas mix.
 * It is also wrong for a stage whose regulator is charged and its valve closed for
 * the whole dive, as the regulator bleeding down looks like breathing, and can be
 * for a 300 bar stage that is carried open but not breathed and cools by 20 °C.
 * @param samples The gas mix in use and the tank pressures at each sample.
 * @param sidemount For each tank, whether it is one of a sidemount pair.
 * @param ngases The number of gas mixes.
 * @return For each tank, the index of its gas mix, or -1 for none.
 */
std::vector<int> guess_tank_gasmixes(const std::vector<tank_gas_sample> &samples,
				     const std::vector<bool> &sidemount, unsigned int ngases)
{
	size_t ntanks = sidemount.size();
	std::vector<std::vector<int>> use(ntanks, std::vector<int>(ngases, 0));
	/**
	 * Whether gasmix is the index of one of the gas mixes.
	 */
	auto valid = [ngases](int gasmix) {
		return gasmix >= 0 && (unsigned int)gasmix < ngases;
	};

	for (size_t tank = 0; tank < ntanks; tank++) {
		// The drops so far, and the gas mix each counted for (-1 for none)
		std::vector<std::pair<int, int>> drops;
		int previous = 0;
		int previous_gasmix = -1;
		bool changed = false;
		for (const tank_gas_sample &sample: samples) {
			if (sample.gasmix != previous_gasmix)
				changed = true;
			int pressure = tank < sample.pressure.size() ? sample.pressure[tank] : 0;
			if (!pressure)
				continue;
			if (previous && pressure < previous) {
				int gasmix = !changed && valid(sample.gasmix) ? sample.gasmix : -1;
				drops.push_back({ gasmix, previous - pressure });
				if (gasmix >= 0)
					use[tank][gasmix] += previous - pressure;
			} else if (previous && pressure > previous) {
				int rise = pressure - previous;
				while (rise > 0 && !drops.empty()) {
					auto &[gasmix, drop] = drops.back();
					int taken = std::min(rise, drop);
					if (gasmix >= 0)
						use[tank][gasmix] -= taken;
					drop -= taken;
					rise -= taken;
					if (!drop)
						drops.pop_back();
				}
			}
			previous = pressure;
			previous_gasmix = sample.gasmix;
			changed = false;
		}
	}

	/**
	 * The gas mix that the use 'u' on each gas mix links to, or -1 for none.
	 */
	auto link = [ngases](const std::vector<int> &u) {
		if (ngases == 0)
			return -1;
		int best = (int)(std::max_element(u.begin(), u.end()) - u.begin());
		int second = 0;
		for (unsigned int gasmix = 0; gasmix < ngases; gasmix++) {
			if ((int)gasmix != best)
				second = std::max(second, u[gasmix]);
		}
		return u[best] >= tank_gas_guess_minimum_use && u[best] >= 2 * second ? best : -1;
	};

	std::vector<int> result(ntanks, -1);
	for (size_t tank = 0; tank < ntanks; tank++)
		result[tank] = link(use[tank]);

	// The tanks of a sidemount pair count together, unless that contradicts the
	// gas mix either of them links to on its own, or either of them lost more on
	// another gas mix than on the one the pair links to.
	if (std::count(sidemount.begin(), sidemount.end(), true) == 2) {
		std::vector<int> pooled(ngases, 0);
		for (size_t tank = 0; tank < ntanks; tank++) {
			if (sidemount[tank]) {
				for (unsigned int gasmix = 0; gasmix < ngases; gasmix++)
					pooled[gasmix] += use[tank][gasmix];
			}
		}
		int pair = link(pooled);
		bool agrees = true;
		for (size_t tank = 0; tank < ntanks; tank++) {
			if (!sidemount[tank])
				continue;
			if (result[tank] >= 0 && result[tank] != pair)
				agrees = false;
			for (unsigned int gasmix = 0; pair >= 0 && gasmix < ngases; gasmix++) {
				if (use[tank][gasmix] > use[tank][pair])
					agrees = false;
			}
		}
		for (size_t tank = 0; tank < ntanks && agrees; tank++) {
			if (sidemount[tank])
				result[tank] = pair;
		}
	}
	return result;
}
