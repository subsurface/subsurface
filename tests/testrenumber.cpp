// SPDX-License-Identifier: GPL-2.0
#include "testrenumber.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/pref.h"
#include <QTextStream>

void TestRenumber::setup()
{
	prefs.cloud_base_url = default_prefs.cloud_base_url;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47.xml", &divelog), 0);
	divelog.process_loaded_dives();
}

void TestRenumber::testMerge()
{
	struct divelog log;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47b.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(divelog.dives.size(), 1);
}

void TestRenumber::testMergeAndAppend()
{
	struct divelog log;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47c.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(divelog.dives.size(), 2);
	QCOMPARE(divelog.dives[1]->number, 2);
}

QTEST_GUILESS_MAIN(TestRenumber)
