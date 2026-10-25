// SPDX-License-Identifier: GPL-2.0
#include "testlibdivecomputer.h"
#include "core/libdivecomputer.h"

// cylinder_use_from_tank_usage() is exercised here with hand-built input
// vectors rather than a binary device fixture, because there is no way to
// reach it with real device data in this test suite:
//  - it is only called from parse_gasmixes(), which in turn is only
//    reachable via a live device download or a binary dive computer log
//    replayed through libdc_buffer_parser();
//  - the raw Shearwater logs in dives/TestDiveShearwaterCloud.db do not use
//    sidemount, and there is no other public sample under dives, nor a FIT
//    sample carrying the Garmin SENSOR_PROFILE field (78) with DC_TANK_USAGE_INCLUDED,
//    DC_TANK_USAGE_EXCLUDED or DC_TANK_USAGE_SIDEMOUNT flags set.
// Calling the helper directly with a truth table lets us cover the decision
// logic without duplicating it or reimplementing parse_gasmixes().

void TestLibdivecomputer::cylinderUseFromTankUsage_data()
{
	QTest::addColumn<int>("currentUse");
	QTest::addColumn<int>("tankUsage");
	QTest::addColumn<int>("divemode");
	QTest::addColumn<int>("ngases");
	QTest::addColumn<int>("ntanks");
	QTest::addColumn<int>("expectedUse");

	QTest::newRow("shearwater sidemount promotes second tank")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_SIDEMOUNT << (int)OC << 1 << 2 << (int)OC_GAS;
	QTest::newRow("garmin included promotes second tank")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_INCLUDED << (int)OC << 1 << 2 << (int)OC_GAS;
	QTest::newRow("excluded tank stays unused")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_EXCLUDED << (int)OC << 1 << 2 << (int)NOT_USED;
	QTest::newRow("excluded overrides gasmix derived use")
		<< (int)OC_GAS << (int)DC_TANK_USAGE_EXCLUDED << (int)OC << 1 << 2 << (int)NOT_USED;
	QTest::newRow("single excluded tank is demoted")
		<< (int)OC_GAS << (int)DC_TANK_USAGE_EXCLUDED << (int)OC << 1 << 1 << (int)NOT_USED;
	QTest::newRow("excluded diluent on ccr is demoted")
		<< (int)DILUENT << (int)DC_TANK_USAGE_EXCLUDED << (int)CCR << 2 << 2 << (int)NOT_USED;
	QTest::newRow("no promotion on ccr sidemount")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_SIDEMOUNT << (int)CCR << 1 << 2 << (int)NOT_USED;
	QTest::newRow("no promotion on ccr included")
		<< (int)DILUENT << (int)DC_TANK_USAGE_INCLUDED << (int)CCR << 1 << 2 << (int)DILUENT;
	QTest::newRow("no promotion with two gas mixes")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_SIDEMOUNT << (int)OC << 2 << 2 << (int)NOT_USED;
	QTest::newRow("no promotion with three tanks")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_SIDEMOUNT << (int)OC << 1 << 3 << (int)NOT_USED;
	QTest::newRow("no promotion with one tank")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_INCLUDED << (int)OC << 1 << 1 << (int)NOT_USED;
	QTest::newRow("unflagged tank stays unused")
		<< (int)NOT_USED << (int)DC_TANK_USAGE_NONE << (int)OC << 1 << 2 << (int)NOT_USED;
	QTest::newRow("unflagged tank keeps gasmix derived use")
		<< (int)OC_GAS << (int)DC_TANK_USAGE_NONE << (int)OC << 1 << 2 << (int)OC_GAS;
	QTest::newRow("sidemount never overrides gasmix derived use")
		<< (int)OC_GAS << (int)DC_TANK_USAGE_SIDEMOUNT << (int)OC << 1 << 2 << (int)OC_GAS;
	QTest::newRow("included never overrides gasmix derived use")
		<< (int)DILUENT << (int)DC_TANK_USAGE_INCLUDED << (int)OC << 1 << 2 << (int)DILUENT;
}

void TestLibdivecomputer::cylinderUseFromTankUsage()
{
	QFETCH(int, currentUse);
	QFETCH(int, tankUsage);
	QFETCH(int, divemode);
	QFETCH(int, ngases);
	QFETCH(int, ntanks);
	QFETCH(int, expectedUse);

	enum cylinderuse result = cylinder_use_from_tank_usage((enum cylinderuse)currentUse,
								(dc_tank_usage_t)tankUsage,
								(enum divemode_t)divemode,
								(unsigned int)ngases,
								(unsigned int)ntanks);
	QCOMPARE((int)result, expectedUse);
}

QTEST_GUILESS_MAIN(TestLibdivecomputer)
