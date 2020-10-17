// SPDX-License-Identifier: GPL-2.0
#include "testdivesiteduplication.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"

void TestDiveSiteDuplication::testReadV2()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/TwoTimesTwo.ssrf", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(dive_site_table.nr, 2);
}

QTEST_GUILESS_MAIN(TestDiveSiteDuplication)
