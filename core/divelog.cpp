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
	*sites = empty_dive_site_table;
}

divelog::~divelog()
{
	clear_dive_table(dives);
	clear_trip_table(trips);
	clear_dive_site_table(sites);
	delete dives;
	delete trips;
	delete sites;
	delete devices;
	delete filter_presets;
}

divelog::divelog(divelog &&log) :
	dives(new dive_table),
	trips(new trip_table),
	sites(new dive_site_table),
	devices(new device_table),
	filter_presets(new filter_preset_table)
{
	*dives = empty_dive_table;
	*trips = empty_trip_table;
	*sites = empty_dive_site_table;
	move_dive_table(log.dives, dives);
	move_trip_table(log.trips, trips);
	move_dive_site_table(log.sites, sites);
	*devices = std::move(*log.devices);
	*filter_presets = std::move(*log.filter_presets);
}

struct divelog &divelog::operator=(divelog &&log)
{
	move_dive_table(log.dives, dives);
	move_trip_table(log.trips, trips);
	move_dive_site_table(log.sites, sites);
	*devices = std::move(*log.devices);
	*filter_presets = std::move(*log.filter_presets);
	return *this;
}

void divelog::clear()
{
	while (dives->nr)
		delete_single_dive(0);
	while (sites->nr)
		delete_dive_site(get_dive_site(0, sites), sites);
	if (trips->nr != 0) {
		report_info("Warning: trip table not empty in divelog::clear()!");
		trips->nr = 0;
	}
	clear_device_table(devices);
	filter_presets->clear();
}

extern "C" void clear_divelog(struct divelog *log)
{
	log->clear();
}
