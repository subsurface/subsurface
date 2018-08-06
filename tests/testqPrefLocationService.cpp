// SPDX-License-Identifier: GPL-2.0
#include "testqPrefLocationService.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefLocationService::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefLocationService");
}

void TestQPrefLocationService::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefLocationService::instance();

	prefs.distance_threshold = 1000;
	prefs.time_threshold = 60;

	QCOMPARE(tst->distance_threshold(), prefs.distance_threshold);
	QCOMPARE(tst->time_threshold(), prefs.time_threshold);
}

void TestQPrefLocationService::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefLocationService::instance();

	tst->set_distance_threshold(2000);
	tst->set_time_threshold(90);

	QCOMPARE(prefs.distance_threshold, 2000);
	QCOMPARE(prefs.time_threshold, 90);
}

void TestQPrefLocationService::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefLocationService::instance();

	tst->set_distance_threshold(2001);
	tst->set_time_threshold(91);

	prefs.distance_threshold = 1000;
	prefs.time_threshold = 60;

	tst->load();
	QCOMPARE(tst->distance_threshold(), 2001);
	QCOMPARE(tst->time_threshold(), 91);
}

void TestQPrefLocationService::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefLocationService::instance();

	prefs.distance_threshold = 1002;
	prefs.time_threshold = 62;

	tst->sync();
	prefs.distance_threshold = 12;
	prefs.time_threshold = 2;

	tst->load();
	QCOMPARE(tst->distance_threshold(), 1002);
	QCOMPARE(tst->time_threshold(), 62);
}

void TestQPrefLocationService::test_multiple()
{
	// test multiple instances have the same information

	prefs.distance_threshold = 52;
	auto tst_direct = new qPrefLocationService;

	prefs.time_threshold = 62;
	auto tst = qPrefLocationService::instance();

	QCOMPARE(tst->distance_threshold(), tst_direct->distance_threshold());
	QCOMPARE(tst->time_threshold(), tst_direct->time_threshold());
	QCOMPARE(tst_direct->distance_threshold(), 52);
	QCOMPARE(tst_direct->time_threshold(), 62);
}

QTEST_MAIN(TestQPrefLocationService)
