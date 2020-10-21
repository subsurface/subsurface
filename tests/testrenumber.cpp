// SPDX-License-Identifier: GPL-2.0
#include "testrenumber.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include <QTextStream>

void TestRenumber::setup()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47.xml", &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
}

void TestRenumber::testMerge()
{
	struct dive_table table = empty_dive_table;
	struct trip_table trips = empty_trip_table;
	struct dive_site_table sites = empty_dive_site_table;
	struct device_table devices;
	struct filter_preset_table filter_presets;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47b.xml", &table, &trips, &sites, &devices, &filter_presets), 0);
	add_imported_dives(&table, &trips, &sites, &devices, IMPORT_MERGE_ALL_TRIPS);
	QCOMPARE(dive_table.nr, 1);
}

void TestRenumber::testMergeAndAppend()
{
	struct dive_table table = empty_dive_table;
	struct trip_table trips = empty_trip_table;
	struct dive_site_table sites = empty_dive_site_table;
	struct device_table devices;
	struct filter_preset_table filter_presets;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47c.xml", &table, &trips, &sites, &devices, &filter_presets), 0);
	add_imported_dives(&table, &trips, &sites, &devices, IMPORT_MERGE_ALL_TRIPS);
	QCOMPARE(dive_table.nr, 2);
	struct dive *d = get_dive(1);
	QVERIFY(d != NULL);
	if (d)
		QCOMPARE(d->number, 2);
}

QTEST_GUILESS_MAIN(TestRenumber)
