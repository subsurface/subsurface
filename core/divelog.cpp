// SPDX-License-Identifier: GPL-2.0
#include "divelog.h"
#include "divelist.h"
#include "divesite.h"
#include "device.h"
#include "errorhelper.h"
#include "filterpreset.h"
#include "trip.h"

struct divelog divelog;

divelog::divelog() :
	dives(std::make_unique<dive_table>()),
	trips(std::make_unique<trip_table>()),
	sites(std::make_unique<dive_site_table>()),
	devices(std::make_unique<device_table>()),
	filter_presets(std::make_unique<filter_preset_table>()),
	autogroup(false)
{
	*dives = empty_dive_table;
	*trips = empty_trip_table;
}

divelog::~divelog()
{
	if (dives)
		clear_dive_table(dives.get());
	if (trips)
		clear_trip_table(trips.get());
}

divelog::divelog(divelog &&) = default;
struct divelog &divelog::operator=(divelog &&) = default;

/* this implements the mechanics of removing the dive from the
 * dive log and the trip, but doesn't deal with updating dive trips, etc */
void divelog::delete_single_dive(int idx)
{
	if (idx < 0 || idx > dives->nr) {
		report_info("Warning: deleting unexisting dive with index %d", idx);
		return;
	}
	struct dive *dive = dives->dives[idx];
	remove_dive_from_trip(dive, trips.get());
	unregister_dive_from_dive_site(dive);
	delete_dive_from_table(dives.get(), idx);
}

void divelog::clear()
{
	while (dives->nr > 0)
		delete_single_dive(dives->nr - 1);
	sites->clear();
	if (trips->nr != 0) {
		report_info("Warning: trip table not empty in divelog::clear()!");
		trips->nr = 0;
	}
	clear_device_table(devices.get());
	filter_presets->clear();
}
