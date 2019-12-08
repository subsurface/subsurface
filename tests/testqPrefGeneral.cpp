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
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefGeneral");
	qPref::registerQML(NULL);
}

void TestQPrefGeneral::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefGeneral::instance();

	prefs.default_filename = copy_qstring("new base12");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;
	prefs.use_default_file = true;

	QCOMPARE(tst->default_filename(), QString(prefs.default_filename));
	QCOMPARE(tst->default_file_behavior(), prefs.default_file_behavior);
	QCOMPARE(tst->defaultsetpoint(), prefs.defaultsetpoint);
	QCOMPARE(tst->o2consumption(), prefs.o2consumption);
	QCOMPARE(tst->pscr_ratio(), prefs.pscr_ratio);
	QCOMPARE(tst->use_default_file(), prefs.use_default_file);
}

void TestQPrefGeneral::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefGeneral::instance();

	tst->set_default_filename("new base22");
	tst->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	tst->set_defaultsetpoint(24);
	tst->set_o2consumption(27);
	tst->set_pscr_ratio(28);
	tst->set_use_default_file(false);
	tst->set_diveshareExport_uid("uid1");
	tst->set_diveshareExport_private(false);

	QCOMPARE(QString(prefs.default_filename), QString("new base22"));
	QCOMPARE(prefs.default_file_behavior, LOCAL_DEFAULT_FILE);
	QCOMPARE(prefs.defaultsetpoint, 24);
	QCOMPARE(prefs.o2consumption, 27);
	QCOMPARE(prefs.pscr_ratio, 28);
	QCOMPARE(prefs.use_default_file, false);
	QCOMPARE(tst->diveshareExport_uid(), QString("uid1"));
	QCOMPARE(tst->diveshareExport_private(), false);
}

void TestQPrefGeneral::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefGeneral::instance();

	tst->set_default_filename("new base32");
	tst->set_default_file_behavior(NO_DEFAULT_FILE);
	tst->set_defaultsetpoint(34);
	tst->set_o2consumption(37);
	tst->set_pscr_ratio(38);
	tst->set_use_default_file(true);
	tst->set_diveshareExport_uid("uid2");
	tst->set_diveshareExport_private(true);

	prefs.default_filename = copy_qstring("error");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;
	prefs.use_default_file = false;

	tst->load();
	QCOMPARE(QString(prefs.default_filename), QString("new base32"));
	QCOMPARE(prefs.default_file_behavior, NO_DEFAULT_FILE);
	QCOMPARE(prefs.defaultsetpoint, 34);
	QCOMPARE(prefs.o2consumption, 37);
	QCOMPARE(prefs.pscr_ratio, 38);
	QCOMPARE(prefs.use_default_file, true);
	QCOMPARE(tst->diveshareExport_uid(), QString("uid2"));
	QCOMPARE(tst->diveshareExport_private(), true);
}

void TestQPrefGeneral::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefGeneral::instance();

	prefs.default_filename = copy_qstring("base42");
	prefs.default_file_behavior = CLOUD_DEFAULT_FILE;
	prefs.defaultsetpoint = 44;
	prefs.o2consumption = 47;
	prefs.pscr_ratio = 48;
	prefs.use_default_file = true;

	tst->sync();
	prefs.default_filename = copy_qstring("error");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.defaultsetpoint = 14;
	prefs.o2consumption = 17;
	prefs.pscr_ratio = 18;
	prefs.use_default_file = false;

	tst->load();
	QCOMPARE(QString(prefs.default_filename), QString("base42"));
	QCOMPARE(prefs.default_file_behavior, CLOUD_DEFAULT_FILE);
	QCOMPARE(prefs.defaultsetpoint, 44);
	QCOMPARE(prefs.o2consumption, 47);
	QCOMPARE(prefs.pscr_ratio, 48);
	QCOMPARE(prefs.use_default_file, true);
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

	general->set_default_filename("filename");
	general->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	general->set_defaultsetpoint(0);
	general->set_o2consumption(0);
	general->set_pscr_ratio(0);
	general->set_use_default_file(true);

	TEST(general->default_filename(), QStringLiteral("filename"));
	TEST(general->default_file_behavior(), LOCAL_DEFAULT_FILE); // since we have a default file, here it returns
	TEST(general->defaultsetpoint(), 0);
	TEST(general->o2consumption(), 0);
	TEST(general->pscr_ratio(), 0);
	TEST(general->use_default_file(), true);

	general->set_default_filename("no_file_name");
	//TODOl: Change this to a enum.
	general->set_default_file_behavior(CLOUD_DEFAULT_FILE);

	general->set_defaultsetpoint(1);
	general->set_o2consumption(1);
	general->set_pscr_ratio(1);
	general->set_use_default_file(false);

	TEST(general->default_filename(), QStringLiteral("no_file_name"));
	TEST(general->default_file_behavior(), CLOUD_DEFAULT_FILE);
	TEST(general->defaultsetpoint(), 1);
	TEST(general->o2consumption(), 1);
	TEST(general->pscr_ratio(), 1);
	TEST(general->use_default_file(), false);
}

void TestQPrefGeneral::test_signals()
{
	QSignalSpy spy3(qPrefGeneral::instance(), &qPrefGeneral::default_filenameChanged);
	QSignalSpy spy4(qPrefGeneral::instance(), &qPrefGeneral::default_file_behaviorChanged);
	QSignalSpy spy5(qPrefGeneral::instance(), &qPrefGeneral::defaultsetpointChanged);
	QSignalSpy spy9(qPrefGeneral::instance(), &qPrefGeneral::o2consumptionChanged);
	QSignalSpy spy10(qPrefGeneral::instance(), &qPrefGeneral::pscr_ratioChanged);
	QSignalSpy spy11(qPrefGeneral::instance(), &qPrefGeneral::use_default_fileChanged);
	QSignalSpy spy12(qPrefGeneral::instance(), &qPrefGeneral::diveshareExport_uidChanged);
	QSignalSpy spy13(qPrefGeneral::instance(), &qPrefGeneral::diveshareExport_privateChanged);

	qPrefGeneral::set_default_filename("new base22");
	qPrefGeneral::set_default_file_behavior(LOCAL_DEFAULT_FILE);
	qPrefGeneral::set_defaultsetpoint(24);
	qPrefGeneral::set_o2consumption(27);
	qPrefGeneral::set_pscr_ratio(28);
	qPrefGeneral::set_use_default_file(false);
	qPrefGeneral::set_diveshareExport_uid("uid1");
	qPrefGeneral::set_diveshareExport_private(false);

	qPrefGeneral::set_default_filename("new base22");
	qPrefGeneral::set_default_file_behavior(LOCAL_DEFAULT_FILE);
	qPrefGeneral::set_defaultsetpoint(24);
	qPrefGeneral::set_o2consumption(27);
	qPrefGeneral::set_pscr_ratio(28);
	qPrefGeneral::set_use_default_file(false);
	qPrefGeneral::set_diveshareExport_uid("uid1");
	qPrefGeneral::set_diveshareExport_private(false);
}

QTEST_MAIN(TestQPrefGeneral)
