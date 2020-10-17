// SPDX-License-Identifier: GPL-2.0
#include "testAirPressure.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/errorhelper.h"
#include <QString>
#include <core/qthelper.h>

void TestAirPressure::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);
}

void TestAirPressure::get_dives()
{
	struct dive *dive;
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/TestAtmPress.xml", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	dive = get_dive(0);
	dive->selected = true;
	QVERIFY(dive != NULL);
}

void TestAirPressure::testReadAirPressure()
{
	struct dive *dive;
	dive = get_dive(0);
	QVERIFY(dive != NULL);
	dive->selected = true;
	QCOMPARE(1012, dive->surface_pressure.mbar);
	dive = get_dive(1);
	QVERIFY(dive != NULL);
	dive->selected = true;
	QCOMPARE(991, dive->surface_pressure.mbar);
}

void TestAirPressure::testConvertAltitudetoAirPressure()
{
	QCOMPARE(891,altitude_to_pressure(1000000)); // 1000 m altitude in mm
	QCOMPARE(1013,altitude_to_pressure(0)); // sea level
}

void TestAirPressure::testWriteReadBackAirPressure()
{
	struct dive *dive;
	int32_t ap = 1111;
	dive = get_dive(0);
	QVERIFY(dive != NULL);
	dive->selected = true;
	dive->surface_pressure.mbar = ap;
	QCOMPARE(save_dives("./testout.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file("./testout.ssrf", &dive_table, &trip_table, &dive_site_table,
			    &device_table, &filter_preset_table), 0);
	dive = get_dive(0);
	QVERIFY(dive != NULL);
	dive->selected = true;
	QCOMPARE(ap, dive->surface_pressure.mbar);
}

QTEST_GUILESS_MAIN(TestAirPressure)
