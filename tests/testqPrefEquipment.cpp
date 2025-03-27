// SPDX-License-Identifier: GPL-2.0
#include "testqPrefEquipment.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefEquipment.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefEquipment::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefEquipment");
	qPref::registerQML(NULL);
}

void TestQPrefEquipment::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefEquipment::instance();
	prefs.default_cylinder = "new base11";
	QCOMPARE(tst->default_cylinder(), QString::fromStdString(prefs.default_cylinder));
	prefs.include_unused_tanks = true;
	QCOMPARE(tst->include_unused_tanks(), prefs.include_unused_tanks);
}

void TestQPrefEquipment::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefEquipment::instance();
	tst->set_default_cylinder("new base21");
	QCOMPARE(QString::fromStdString(prefs.default_cylinder), QString("new base21"));
	tst->set_include_unused_tanks(false);
	QCOMPARE(prefs.include_unused_tanks, false);
}

void TestQPrefEquipment::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefEquipment::instance();

	tst->set_default_cylinder("new base31");
	prefs.default_cylinder = "error";
	tst->set_include_unused_tanks(false);
	prefs.include_unused_tanks = true;
	tst->load();
	QCOMPARE(QString::fromStdString(prefs.default_cylinder), QString("new base31"));
	QCOMPARE(prefs.include_unused_tanks, false);
}

void TestQPrefEquipment::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefEquipment::instance();
	prefs.default_cylinder = "base41";
	prefs.include_unused_tanks = true;

	tst->sync();
	prefs.default_cylinder = "error";
	prefs.include_unused_tanks = false;

	tst->load();
	QCOMPARE(QString::fromStdString(prefs.default_cylinder), QString("base41"));
	QCOMPARE(prefs.include_unused_tanks, true);

}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	equipment->sync();           \
	equipment->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefEquipment::test_oldPreferences()
{
	auto equipment = qPrefEquipment::instance();
	equipment->set_default_cylinder("cylinder_2");
	TEST(equipment->default_cylinder(), QStringLiteral("cylinder_2"));
	equipment->set_default_cylinder("cylinder_1");
	TEST(equipment->default_cylinder(), QStringLiteral("cylinder_1"));
	equipment->set_include_unused_tanks(true);
	TEST(equipment->include_unused_tanks(), true);
	equipment->set_include_unused_tanks(false);
	TEST(equipment->include_unused_tanks(), false);
}

void TestQPrefEquipment::test_signals()
{
	qPrefEquipment::set_default_cylinder("signal test");
	QSignalSpy spy1(qPrefEquipment::instance(), &qPrefEquipment::default_cylinderChanged);
	QSignalSpy spy2(qPrefEquipment::instance(), &qPrefEquipment::include_unused_tanksChanged);

	// set default cylinder to same value it already had
	qPrefEquipment::set_default_cylinder("signal test");
	QCOMPARE(spy1.count(), 0);

	// change default cylinder to different value
	qPrefEquipment::set_default_cylinder("different value");
	QCOMPARE(spy1.count(), 1);
	QVERIFY(spy1.takeFirst().at(0).toBool() == true);

	prefs.include_unused_tanks = true;
	qPrefEquipment::set_include_unused_tanks(false);
	QCOMPARE(spy2.count(), 1);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
}

QTEST_MAIN(TestQPrefEquipment)
