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
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefEquipment");
	qPref::registerQML(NULL);
}

void TestQPrefEquipment::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefEquipment::instance();
	prefs.default_cylinder = copy_qstring("new base11");
	QCOMPARE(tst->default_cylinder(), QString(prefs.default_cylinder));
	prefs.display_unused_tanks = true;
	QCOMPARE(tst->display_unused_tanks(), prefs.display_unused_tanks);
}

void TestQPrefEquipment::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefEquipment::instance();
	tst->set_default_cylinder("new base21");
	QCOMPARE(QString(prefs.default_cylinder), QString("new base21"));
	tst->set_display_unused_tanks(false);
	QCOMPARE(prefs.display_unused_tanks, false);
}

void TestQPrefEquipment::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefEquipment::instance();

	tst->set_default_cylinder("new base31");
	prefs.default_cylinder = copy_qstring("error");
	tst->set_display_unused_tanks(false);
	prefs.display_unused_tanks = true;
	tst->load();
	QCOMPARE(QString(prefs.default_cylinder), QString("new base31"));
	QCOMPARE(prefs.display_unused_tanks, false);
}

void TestQPrefEquipment::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefEquipment::instance();
	prefs.default_cylinder = copy_qstring("base41");
	prefs.display_unused_tanks = true;

	tst->sync();
	prefs.default_cylinder = copy_qstring("error");
	prefs.display_unused_tanks = false;

	tst->load();
	QCOMPARE(QString(prefs.default_cylinder), QString("base41"));
	QCOMPARE(prefs.display_unused_tanks, true);

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
	equipment->set_display_unused_tanks(true);
	TEST(equipment->display_unused_tanks(), true);
	equipment->set_display_unused_tanks(false);
	TEST(equipment->display_unused_tanks(), false);
}

void TestQPrefEquipment::test_signals()
{
	QSignalSpy spy1(qPrefEquipment::instance(), &qPrefEquipment::default_cylinderChanged);
	QSignalSpy spy2(qPrefEquipment::instance(), &qPrefEquipment::display_unused_tanksChanged);

	qPrefEquipment::set_default_cylinder("new base21");
	QCOMPARE(spy1.count(), 1);
	QVERIFY(spy1.takeFirst().at(0).toBool() == false);

	prefs.display_unused_tanks = true;
	qPrefEquipment::set_display_unused_tanks(false);
	QCOMPARE(spy2.count(), 1);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
}

QTEST_MAIN(TestQPrefEquipment)
