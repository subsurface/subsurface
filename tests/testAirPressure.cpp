// SPDX-License-Identifier: GPL-2.0
#include "testAirPressure.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/errorhelper.h"
#include <QString>
#include <core/qthelper.h>

void TestAirPressure::initTestCase()
{
	TestBase::initTestCase();
	prefs.cloud_base_url = default_prefs.cloud_base_url;
}

void TestAirPressure::get_dives()
{
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/TestAtmPress.xml", &divelog), 0);
	QVERIFY(divelog.dives.size() >= 1);
}

void TestAirPressure::testReadAirPressure()
{
	QVERIFY(divelog.dives.size() >= 2);
	QCOMPARE(1012, divelog.dives[0]->surface_pressure.mbar);
	QCOMPARE(991, divelog.dives[1]->surface_pressure.mbar);
}

void TestAirPressure::testConvertAltitudetoAirPressure()
{
	QCOMPARE(891, altitude_to_pressure(1000000).mbar); // 1000 m altitude in mm
	QCOMPARE(1013, altitude_to_pressure(0).mbar); // sea level
}

void TestAirPressure::testWriteReadBackAirPressure()
{
	int32_t ap = 1111;
	QVERIFY(divelog.dives.size() >= 1);
	divelog.dives[0]->surface_pressure.mbar = ap;
	QCOMPARE(save_dives("./testout.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file("./testout.ssrf", &divelog), 0);
	QVERIFY(divelog.dives.size() >= 1);
	QCOMPARE(ap, divelog.dives[0]->surface_pressure.mbar);
}

QTEST_GUILESS_MAIN(TestAirPressure)
