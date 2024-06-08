// SPDX-License-Identifier: GPL-2.0
#include "divelog.h"
#include "divelist.h"
#include "divesite.h"
#include "device.h"
#include "dive.h"
#include "errorhelper.h"
#include "filterpreset.h"
#include "filterpresettable.h"
#include "trip.h"

struct divelog divelog;

divelog::divelog() = default;
divelog::~divelog() = default;
divelog::divelog(divelog &&) = default;
struct divelog &divelog::operator=(divelog &&) = default;

/* this implements the mechanics of removing the dive from the
 * dive log and the trip, but doesn't deal with updating dive trips, etc */
void divelog::delete_single_dive(int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= dives.size()) {
		report_info("Warning: deleting non-existing dive with index %d", idx);
		return;
	}
	struct dive *dive = dives[idx].get();
	struct dive_trip *trip = unregister_dive_from_trip(dive);

	// Deleting a dive may change the order of trips!
	if (trip)
		trips.sort();

	if (trip && trip->dives.empty())
		trips.pull(trip);
	unregister_dive_from_dive_site(dive);
	dives.erase(dives.begin() + idx);
}

void divelog::delete_multiple_dives(const std::vector<dive *> &dives_to_delete)
{
	bool trips_changed = false;

	for (dive *d: dives_to_delete) {
		// Delete dive from trip and delete trip if empty
		struct dive_trip *trip = unregister_dive_from_trip(d);
		if (trip && trip->dives.empty()) {
			trips_changed = true;
			trips.pull(trip);
		}

		unregister_dive_from_dive_site(d);
		dives.pull(d);
	}

	// Deleting a dive may change the order of trips!
	if (trips_changed)
		trips.sort();
}

void divelog::clear()
{
	dives.clear();
	sites.clear();
	trips.clear();
	devices.clear();
	filter_presets.clear();
}

/* check if we have a trip right before / after this dive */
bool divelog::is_trip_before_after(const struct dive *dive, bool before) const
{
	auto it = std::find_if(dives.begin(), dives.end(),
			       [dive](auto &d) { return d.get() == dive; });
	if (it == dives.end())
		return false;

	if (before) {
		do {
			if (it == dives.begin())
				return false;
			--it;
		} while ((*it)->invalid);
		return (*it)->divetrip != nullptr;
	} else {
		++it;
		while (it != dives.end() && (*it)->invalid)
			++it;
		return it != dives.end() && (*it)->divetrip != nullptr;
	}
}
