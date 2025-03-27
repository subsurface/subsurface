// SPDX-License-Identifier: GPL-2.0
#include "testqPrefGeneral.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefGeneral.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefGeneral::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefGeneral");
	qPref::registerQML(NULL);
}

void TestQPrefGeneral::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefGeneral::instance();

	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;

	QCOMPARE(tst->defaultsetpoint(), prefs.defaultsetpoint);
	QCOMPARE(tst->o2consumption(), prefs.o2consumption);
	QCOMPARE(tst->pscr_ratio(), prefs.pscr_ratio);
}

void TestQPrefGeneral::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefGeneral::instance();

	tst->set_defaultsetpoint(24);
	tst->set_o2consumption(27);
	tst->set_pscr_ratio(28);
	tst->set_diveshareExport_uid("uid1");
	tst->set_diveshareExport_private(false);

	QCOMPARE(prefs.defaultsetpoint, 24);
	QCOMPARE(prefs.o2consumption, 27);
	QCOMPARE(prefs.pscr_ratio, 28);
	QCOMPARE(tst->diveshareExport_uid(), QString("uid1"));
	QCOMPARE(tst->diveshareExport_private(), false);
}

void TestQPrefGeneral::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefGeneral::instance();

	tst->set_defaultsetpoint(34);
	tst->set_o2consumption(37);
	tst->set_pscr_ratio(38);
	tst->set_diveshareExport_uid("uid2");
	tst->set_diveshareExport_private(true);

	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;

	tst->load();
	QCOMPARE(prefs.defaultsetpoint, 34);
	QCOMPARE(prefs.o2consumption, 37);
	QCOMPARE(prefs.pscr_ratio, 38);
	QCOMPARE(tst->diveshareExport_uid(), QString("uid2"));
	QCOMPARE(tst->diveshareExport_private(), true);
}

void TestQPrefGeneral::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefGeneral::instance();

	prefs.defaultsetpoint = 44;
	prefs.o2consumption = 47;
	prefs.pscr_ratio = 48;

	tst->sync();
	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;

	tst->load();
	QCOMPARE(prefs.defaultsetpoint, 44);
	QCOMPARE(prefs.o2consumption, 47);
	QCOMPARE(prefs.pscr_ratio, 48);
}

void TestQPrefGeneral::test_multiple()
{
	// test multiple instances have the same information

	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;
	auto tst = qPrefGeneral::instance();

	QCOMPARE(tst->o2consumption(), qPrefGeneral::o2consumption());
	QCOMPARE(tst->pscr_ratio(), qPrefGeneral::pscr_ratio());
	QCOMPARE(qPrefGeneral::o2consumption(), 17);
	QCOMPARE(qPrefGeneral::pscr_ratio(), 18);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	general->sync();           \
	general->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefGeneral::test_oldPreferences()
{
	auto general = qPrefGeneral::instance();

	general->set_defaultsetpoint(0);
	general->set_o2consumption(0);
	general->set_pscr_ratio(0);

	TEST(general->defaultsetpoint(), 0);
	TEST(general->o2consumption(), 0);
	TEST(general->pscr_ratio(), 0);

	general->set_defaultsetpoint(1);
	general->set_o2consumption(1);
	general->set_pscr_ratio(1);

	TEST(general->defaultsetpoint(), 1);
	TEST(general->o2consumption(), 1);
	TEST(general->pscr_ratio(), 1);
}

void TestQPrefGeneral::test_signals()
{
	QSignalSpy spy5(qPrefGeneral::instance(), &qPrefGeneral::defaultsetpointChanged);
	QSignalSpy spy9(qPrefGeneral::instance(), &qPrefGeneral::o2consumptionChanged);
	QSignalSpy spy10(qPrefGeneral::instance(), &qPrefGeneral::pscr_ratioChanged);
	QSignalSpy spy12(qPrefGeneral::instance(), &qPrefGeneral::diveshareExport_uidChanged);
	QSignalSpy spy13(qPrefGeneral::instance(), &qPrefGeneral::diveshareExport_privateChanged);

	qPrefGeneral::set_defaultsetpoint(24);
	qPrefGeneral::set_o2consumption(27);
	qPrefGeneral::set_pscr_ratio(28);
	qPrefGeneral::set_diveshareExport_uid("uid1");
	qPrefGeneral::set_diveshareExport_private(false);

	qPrefGeneral::set_defaultsetpoint(24);
	qPrefGeneral::set_o2consumption(27);
	qPrefGeneral::set_pscr_ratio(28);
	qPrefGeneral::set_diveshareExport_uid("uid1");
	qPrefGeneral::set_diveshareExport_private(false);
}

QTEST_MAIN(TestQPrefGeneral)
