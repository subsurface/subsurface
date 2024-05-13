// SPDX-License-Identifier: GPL-2.0
#include "divelog.h"
#include "divelist.h"
#include "divesite.h"
#include "device.h"
#include "errorhelper.h"
#include "filterpreset.h"
#include "trip.h"

struct divelog divelog;

// We can't use smart pointers, since this is used from C
// and it would be bold to presume that std::unique_ptr<>
// and a plain pointer have the same memory layout.
divelog::divelog() :
	dives(new dive_table),
	trips(new trip_table),
	sites(new dive_site_table),
	devices(new device_table),
	filter_presets(new filter_preset_table),
	autogroup(false)
{
	*dives = empty_dive_table;
	*trips = empty_trip_table;
}

divelog::~divelog()
{
	clear_dive_table(dives);
	clear_trip_table(trips);
	delete dives;
	delete trips;
	delete sites;
	delete devices;
	delete filter_presets;
}

divelog::divelog(divelog &&log) :
	dives(new dive_table),
	trips(new trip_table),
	sites(new dive_site_table(std::move(*log.sites))),
	devices(new device_table),
	filter_presets(new filter_preset_table)
{
	*dives = empty_dive_table;
	*trips = empty_trip_table;
	move_dive_table(log.dives, dives);
	move_trip_table(log.trips, trips);
	*devices = std::move(*log.devices);
	*filter_presets = std::move(*log.filter_presets);
}

struct divelog &divelog::operator=(divelog &&log)
{
	move_dive_table(log.dives, dives);
	move_trip_table(log.trips, trips);
	*sites = std::move(*log.sites);
	*devices = std::move(*log.devices);
	*filter_presets = std::move(*log.filter_presets);
	return *this;
}

/* this implements the mechanics of removing the dive from the
 * dive log and the trip, but doesn't deal with updating dive trips, etc */
void divelog::delete_single_dive(int idx)
{
	if (idx < 0 || idx > dives->nr) {
		report_info("Warning: deleting unexisting dive with index %d", idx);
		return;
	}
	struct dive *dive = dives->dives[idx];
	remove_dive_from_trip(dive, trips);
	unregister_dive_from_dive_site(dive);
	delete_dive_from_table(dives, idx);
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
	clear_device_table(devices);
	filter_presets->clear();
}
