// SPDX-License-Identifier: GPL-2.0
#include "testlibdivecomputer.h"
#include "core/libdivecomputer.h"
#include "core/qthelper.h"

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

// AI-generated (Claude)
/**
 * @brief matchTanksToGasmixes_data lists the rows for matchTanksToGasmixes()
 * match_tanks_to_gasmixes() is tested with hand-built input for the same reason:
 * no dive under dives links a tank to a gas mix other than the one at its index.
 * The tanks are the gas mix each tank is linked to (-1 for none), and the result
 * is gasmix/tank for each cylinder (-1 for none), and then the cylinder that
 * gasmix_cylinders() gives each gas mix, for the gas changes.
 */
void TestLibdivecomputer::matchTanksToGasmixes_data()
{
	QTest::addColumn<QString>("tanks");
	QTest::addColumn<int>("ngases");
	QTest::addColumn<QString>("expected");
	QTest::addColumn<QString>("expectedGasmixCylinders");

	QTest::newRow("no tanks") << "" << 2 << "0/-1 1/-1" << "0 1";
	QTest::newRow("no links, more tanks than gas mixes") << "-1,-1,-1" << 2 << "0/0 1/1 -1/2" << "0 1";
	QTest::newRow("no links, more gas mixes than tanks") << "-1" << 3 << "0/0 1/-1 2/-1" << "0 1 2";
	QTest::newRow("each tank on the gas mix at its index") << "0,1" << 3 << "0/0 1/1 2/-1" << "0 1 2";
	QTest::newRow("link to a gas mix that doesn't exist") << "5,-1" << 2 << "0/0 1/1" << "0 1";
	QTest::newRow("sidemount pair and two stages") << "2,2,1,0" << 3 << "2/0 2/1 1/2 0/3" << "3 2 0";
	QTest::newRow("sidemount pair on the first gas mix") << "0,0" << 2 << "0/0 0/1 1/-1" << "0 2";
	QTest::newRow("all tanks on one gas mix") << "1,1,1" << 2 << "1/0 1/1 1/2 0/-1" << "3 0";
	QTest::newRow("tank without a link among links") << "2,-1" << 3 << "2/0 -1/1 0/-1 1/-1" << "2 3 0";
	QTest::newRow("link to a gas mix that doesn't exist among links") << "1,7" << 2 << "1/0 -1/1 0/-1" << "2 0";
}

// AI-generated (Claude)
/**
 * @brief matchTanksToGasmixes checks the cylinders match_tanks_to_gasmixes() makes
 * from one row of links, and the cylinder gasmix_cylinders() gives each gas mix.
 */
void TestLibdivecomputer::matchTanksToGasmixes()
{
	QFETCH(QString, tanks);
	QFETCH(int, ngases);
	QFETCH(QString, expected);
	QFETCH(QString, expectedGasmixCylinders);

	std::vector<int> tank_gasmix;
	for (const QString &tank: tanks.split(',', SKIP_EMPTY))
		tank_gasmix.push_back(tank.toInt());
	std::vector<cylinder_source> sources = match_tanks_to_gasmixes(tank_gasmix, (unsigned int)ngases);
	QStringList result;
	for (const cylinder_source &source: sources)
		result.append(QString("%1/%2").arg(source.gasmix).arg(source.tank));
	QCOMPARE(result.join(' '), expected);
	QStringList cylinders;
	for (int cylinder: gasmix_cylinders(sources, (unsigned int)ngases))
		cylinders.append(QString::number(cylinder));
	QCOMPARE(cylinders.join(' '), expectedGasmixCylinders);
}

// AI-generated (Claude)
/**
 * @brief gasmixCylindersPreferUsed checks that the gas changes go to a used
 * cylinder with the gas mix, e.g. not to a buddy's tank the dive computer
 * excludes, if there is one.
 */
void TestLibdivecomputer::gasmixCylindersPreferUsed()
{
	const std::vector<cylinder_source> sources = { { 0, 0 }, { 0, 1 }, { 1, -1 } };
	QCOMPARE(gasmix_cylinders(sources, 2), std::vector<int>({ 0, 2 }));
	QCOMPARE(gasmix_cylinders(sources, 2, { false, true, true }), std::vector<int>({ 1, 2 }));
	QCOMPARE(gasmix_cylinders(sources, 2, { false, false, true }), std::vector<int>({ 0, 2 }));
	QCOMPARE(gasmix_cylinders(sources, 2, { true, true, false }), std::vector<int>({ 0, 2 }));
}

QTEST_GUILESS_MAIN(TestLibdivecomputer)
