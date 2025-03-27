// SPDX-License-Identifier: GPL-2.0
#include "testqPrefPartialPressureGas.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefPartialPressureGas.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefPartialPressureGas::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefPartialPressureGas");
}

void TestQPrefPartialPressureGas::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefPartialPressureGas::instance();

	prefs.pp_graphs.phe = true;
	prefs.pp_graphs.phe_threshold = 21.2;
	prefs.pp_graphs.pn2 = true;
	prefs.pp_graphs.pn2_threshold = 21.3;
	prefs.pp_graphs.po2 = true;
	prefs.pp_graphs.po2_threshold_max = 21.4;
	prefs.pp_graphs.po2_threshold_min = 21.5;

	QCOMPARE(tst->phe(), prefs.pp_graphs.phe);
	QCOMPARE(tst->phe_threshold(), prefs.pp_graphs.phe_threshold);
	QCOMPARE(tst->pn2(), prefs.pp_graphs.pn2);
	QCOMPARE(tst->pn2_threshold(), prefs.pp_graphs.pn2_threshold);
	QCOMPARE(tst->po2(), prefs.pp_graphs.po2);
	QCOMPARE(tst->po2_threshold_max(), prefs.pp_graphs.po2_threshold_max);
	QCOMPARE(tst->po2_threshold_min(), prefs.pp_graphs.po2_threshold_min);
}

void TestQPrefPartialPressureGas::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefPartialPressureGas::instance();

	tst->set_phe(false);
	tst->set_phe_threshold(22.2);
	tst->set_pn2(false);
	tst->set_pn2_threshold(22.3);
	tst->set_po2(false);
	tst->set_po2_threshold_max(22.4);
	tst->set_po2_threshold_min(22.5);

	QCOMPARE(prefs.pp_graphs.phe, false);
	QCOMPARE(prefs.pp_graphs.phe_threshold, 22.2);
	QCOMPARE(prefs.pp_graphs.pn2, false);
	QCOMPARE(prefs.pp_graphs.pn2_threshold, 22.3);
	QCOMPARE(prefs.pp_graphs.po2, false);
	QCOMPARE(prefs.pp_graphs.po2_threshold_max, 22.4);
	QCOMPARE(prefs.pp_graphs.po2_threshold_min, 22.5);
}

void TestQPrefPartialPressureGas::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefPartialPressureGas::instance();

	tst->set_phe(true);
	tst->set_phe_threshold(23.2);
	tst->set_pn2(true);
	tst->set_pn2_threshold(23.3);
	tst->set_po2(true);
	tst->set_po2_threshold_max(23.4);
	tst->set_po2_threshold_min(23.5);

	prefs.pp_graphs.phe = false;
	prefs.pp_graphs.phe_threshold = 21.2;
	prefs.pp_graphs.pn2 = false;
	prefs.pp_graphs.pn2_threshold = 21.3;
	prefs.pp_graphs.po2 = false;
	prefs.pp_graphs.po2_threshold_max = 21.4;
	prefs.pp_graphs.po2_threshold_min = 21.5;

	tst->load();
	QCOMPARE(prefs.pp_graphs.phe, true);
	QCOMPARE(prefs.pp_graphs.phe_threshold, 23.2);
	QCOMPARE(prefs.pp_graphs.pn2, true);
	QCOMPARE(prefs.pp_graphs.pn2_threshold, 23.3);
	QCOMPARE(prefs.pp_graphs.po2, true);
	QCOMPARE(prefs.pp_graphs.po2_threshold_max, 23.4);
	QCOMPARE(prefs.pp_graphs.po2_threshold_min, 23.5);
}

void TestQPrefPartialPressureGas::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefPartialPressureGas::instance();

	prefs.pp_graphs.phe = false;
	prefs.pp_graphs.phe_threshold = 24.2;
	prefs.pp_graphs.pn2 = false;
	prefs.pp_graphs.pn2_threshold = 24.3;
	prefs.pp_graphs.po2 = false;
	prefs.pp_graphs.po2_threshold_max = 24.4;
	prefs.pp_graphs.po2_threshold_min = 24.5;

	tst->sync();
	prefs.pp_graphs.phe = true;
	prefs.pp_graphs.phe_threshold = 1.2;
	prefs.pp_graphs.pn2 = true;
	prefs.pp_graphs.pn2_threshold = 1.3;
	prefs.pp_graphs.po2 = true;
	prefs.pp_graphs.po2_threshold_max = 1.4;
	prefs.pp_graphs.po2_threshold_min = 1.5;

	tst->load();
	QCOMPARE(prefs.pp_graphs.phe, false);
	QCOMPARE(prefs.pp_graphs.phe_threshold, 24.2);
	QCOMPARE(prefs.pp_graphs.pn2, false);
	QCOMPARE(prefs.pp_graphs.pn2_threshold, 24.3);
	QCOMPARE(prefs.pp_graphs.po2, false);
	QCOMPARE(prefs.pp_graphs.po2_threshold_max, 24.4);
	QCOMPARE(prefs.pp_graphs.po2_threshold_min, 24.5);
}

void TestQPrefPartialPressureGas::test_multiple()
{
	// test multiple instances have the same information

	prefs.pp_graphs.phe_threshold = 2.2;
	prefs.pp_graphs.pn2_threshold = 2.3;
	auto tst = qPrefPartialPressureGas::instance();

	QCOMPARE(tst->phe_threshold(), qPrefPartialPressureGas::phe_threshold());
	QCOMPARE(tst->pn2_threshold(), qPrefPartialPressureGas::pn2_threshold());
	QCOMPARE(qPrefPartialPressureGas::phe_threshold(), 2.2);
	QCOMPARE(qPrefPartialPressureGas::pn2_threshold(), 2.3);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	pp->sync();           \
	pp->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefPartialPressureGas::test_oldPreferences()
{
	auto pp = qPrefPartialPressureGas::instance();
	pp->set_pn2(false);
	pp->set_phe(false);
	pp->set_po2(false);
	pp->set_po2_threshold_min(1.0);
	pp->set_po2_threshold_max(2.0);
	pp->set_pn2_threshold(3.0);
	pp->set_phe_threshold(4.0);

	TEST(pp->pn2(), false);
	TEST(pp->phe(), false);
	TEST(pp->po2(), false);
	TEST(pp->pn2_threshold(), 3.0);
	TEST(pp->phe_threshold(), 4.0);
	TEST(pp->po2_threshold_min(), 1.0);
	TEST(pp->po2_threshold_max(), 2.0);

	pp->set_pn2(true);
	pp->set_phe(true);
	pp->set_po2(true);
	pp->set_po2_threshold_min(4.0);
	pp->set_po2_threshold_max(5.0);
	pp->set_pn2_threshold(6.0);
	pp->set_phe_threshold(7.0);

	TEST(pp->pn2(), true);
	TEST(pp->phe(), true);
	TEST(pp->po2(), true);
	TEST(pp->pn2_threshold(), 6.0);
	TEST(pp->phe_threshold(), 7.0);
	TEST(pp->po2_threshold_min(), 4.0);
	TEST(pp->po2_threshold_max(), 5.0);

}

void TestQPrefPartialPressureGas::test_signals()
{
	QSignalSpy spy1(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::pheChanged);
	QSignalSpy spy2(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::phe_thresholdChanged);
	QSignalSpy spy3(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::pn2Changed);
	QSignalSpy spy4(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::pn2_thresholdChanged);
	QSignalSpy spy5(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::po2Changed);
	QSignalSpy spy6(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::po2_threshold_maxChanged);
	QSignalSpy spy7(qPrefPartialPressureGas::instance(), &qPrefPartialPressureGas::po2_threshold_minChanged);

	prefs.pp_graphs.phe = true;
	qPrefPartialPressureGas::set_phe(false);
	qPrefPartialPressureGas::set_phe_threshold(-22.2);
	prefs.pp_graphs.pn2 = true;
	qPrefPartialPressureGas::set_pn2(false);
	qPrefPartialPressureGas::set_pn2_threshold(-22.3);
	prefs.pp_graphs.po2 = true;
	qPrefPartialPressureGas::set_po2(false);
	qPrefPartialPressureGas::set_po2_threshold_max(-22.4);
	qPrefPartialPressureGas::set_po2_threshold_min(-22.5);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy7.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toBool() == false);
	QVERIFY(spy2.takeFirst().at(0).toDouble() == -22.2);
	QVERIFY(spy3.takeFirst().at(0).toBool() == false);
	QVERIFY(spy4.takeFirst().at(0).toDouble() == -22.3);
	QVERIFY(spy5.takeFirst().at(0).toBool() == false);
	QVERIFY(spy6.takeFirst().at(0).toDouble() == -22.4);
	QVERIFY(spy7.takeFirst().at(0).toDouble() == -22.5);
}


QTEST_MAIN(TestQPrefPartialPressureGas)
