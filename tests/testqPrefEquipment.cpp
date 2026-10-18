// SPDX-License-Identifier: GPL-2.0
#include "testqPrefEquipment.h"

#include "core/dive.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefEquipment.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSettings>
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

// Regression tests for persisting the global cylinder-type list
// (tank_info_table) edited through the "Manage cylinder types" dialog, so
// that renames, resizes, deletions and reordering survive application
// restarts instead of being rebuilt from the hard-coded defaults on every
// dive log open. Both the persisted QSettings state and the in-process
// cylinder-type baseline cache (core/equipment.cpp) are process-wide, so
// each test below explicitly resets both before setting up its own fixture,
// rather than relying on running in a particular order relative to the
// other tank_info_table tests in this file.
void TestQPrefEquipment::test_tank_info_table_defaults_without_customisation()
{
	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	set_tank_info_baseline(default_tank_info_table());

	qPrefEquipment::instance()->load();
	divelog.clear();
	tank_info_table.clear();
	prefs.display_default_tank_infos = true;
	reset_tank_info_table(tank_info_table);

	// See add_default_tank_infos() in core/equipment.cpp for the hard-coded list.
	QVERIFY(tank_info_table.size() > 10);
	QCOMPARE(tank_info_table[0].name, std::string("10.0L"));
}

void TestQPrefEquipment::test_tank_info_table_persists_across_restart()
{
	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	divelog.clear();
	tank_info_table.clear();

	// save_tank_info_table() persists get_tank_info_baseline(), NOT
	// tank_info_table (which is just the runtime display/picker view - see
	// core/equipment.h); set up the fixture there, the way the "Manage
	// cylinder types" editor's add()/resize actions do (see
	// qt-models/tankinfomodel.cpp).
	set_tank_info_baseline({
		tank_info { "Renamed AL80", 0, 11100, 0, 207 },
		tank_info { "Custom 232 bar", 0, 12000, 0, 232 },
	});

	// This is what ManageTankInfoDialog::done() does when the editor closes.
	save_tank_info_table();

	// Simulate an application restart: wipe the in-memory state, then ask
	// the preference layer to reload from disk exactly as qPref::load()
	// does at startup, and rebuild the table as clear_dive_file_data() does
	// for every opened or newly created dive log.
	tank_info_table.clear();
	qPrefEquipment::instance()->load();
	reset_tank_info_table(tank_info_table);

	QCOMPARE(static_cast<int>(tank_info_table.size()), 2);
	QCOMPARE(tank_info_table[0].name, std::string("Renamed AL80"));
	QCOMPARE(tank_info_table[0].ml, 11100);
	QCOMPARE(tank_info_table[0].bar, 207);
	QCOMPARE(tank_info_table[1].name, std::string("Custom 232 bar"));
	QCOMPARE(tank_info_table[1].ml, 12000);
	QCOMPARE(tank_info_table[1].bar, 232);
}

// Regression test: a corrupted or hand-edited settings file could persist a
// negative "TankTypes/count", which used to be passed straight into
// std::vector::reserve(), where the implicit conversion to size_t turns it
// into a huge allocation attempt instead of falling back sanely.
void TestQPrefEquipment::test_tank_info_table_negative_count_falls_back_to_defaults()
{
	QSettings().setValue(QStringLiteral("Equipment/TankTypes/count"), -1);

	qPrefEquipment::instance()->load();
	divelog.clear();
	tank_info_table.clear();
	prefs.display_default_tank_infos = true;
	reset_tank_info_table(tank_info_table);

	// Falls back to the hard-coded defaults exactly as if nothing had been
	// persisted yet; see add_default_tank_infos() in core/equipment.cpp.
	QVERIFY(tank_info_table.size() > 10);
	QCOMPARE(tank_info_table[0].name, std::string("10.0L"));

	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
}

// A non-numeric count used to be converted to zero silently, replacing the
// baseline with an empty custom list rather than treating the saved value as
// invalid and falling back to the built-in defaults.
void TestQPrefEquipment::test_tank_info_table_malformed_count_falls_back_to_defaults()
{
	QSettings().setValue(QStringLiteral("Equipment/TankTypes/count"), QStringLiteral("not a number"));

	qPrefEquipment::instance()->load();
	divelog.clear();
	tank_info_table.clear();
	prefs.display_default_tank_infos = true;
	reset_tank_info_table(tank_info_table);

	QVERIFY(tank_info_table.size() > 10);
	QCOMPARE(tank_info_table[0].name, std::string("10.0L"));

	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
}

// Regression test: ManageTankInfoDialog::done() calls save_tank_info_table()
// unconditionally, even if the user made no edits. Before this fix that
// persisted a snapshot of the runtime tank_info_table (baseline plus
// whatever the currently open file happens to use), permanently promoting
// every one of the open file's cylinder types into the library. Persisting
// get_tank_info_baseline() instead means a no-op close is a no-op save.
void TestQPrefEquipment::test_tank_info_table_close_without_changes_leaves_settings_unchanged()
{
	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	set_tank_info_baseline({ tank_info { "Renamed AL80", 0, 11100, 0, 207 } });
	save_tank_info_table();

	int countBefore = QSettings().value(QStringLiteral("Equipment/TankTypes/count")).toInt();
	QString nameBefore = QSettings().value(QStringLiteral("Equipment/TankTypes/Name0")).toString();

	// Simulate opening "Manage cylinder types...", making no edits, and
	// closing it: tank_info_table (the editor's unrelated runtime display)
	// ends up with extra entries from the open file, but the baseline -
	// the only thing save_tank_info_table() persists - is untouched.
	divelog.clear();
	tank_info_table.clear();
	auto d = std::make_unique<dive>();
	cylinder_t cyl;
	cyl.type.description = "FileOnlyType";
	cyl.type.size.mliter = 11100;
	cyl.type.workingpressure.mbar = 207000;
	d->cylinders.push_back(cyl);
	divelog.dives.register_dive(std::move(d));
	prefs.display_default_tank_infos = true;
	reset_tank_info_table(tank_info_table);
	QVERIFY(static_cast<int>(tank_info_table.size()) > 1); // baseline entry + FileOnlyType

	save_tank_info_table();

	QCOMPARE(QSettings().value(QStringLiteral("Equipment/TankTypes/count")).toInt(), countBefore);
	QCOMPARE(QSettings().value(QStringLiteral("Equipment/TankTypes/Name0")).toString(), nameBefore);

	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	divelog.clear();
}

// Regression test: with "Show default cylinder sizes" unchecked,
// tank_info_table (built by reset_tank_info_table()) excludes the baseline
// entirely. Before this fix, closing the editor persisted that reduced
// runtime table as the new baseline, permanently discarding the built-in
// defaults even after re-enabling the option. Persisting
// get_tank_info_baseline() directly means the preference only ever affects
// what is displayed, never what is persisted.
void TestQPrefEquipment::test_tank_info_table_defaults_unchecked_does_not_overwrite_baseline()
{
	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	set_tank_info_baseline(default_tank_info_table());
	divelog.clear();
	tank_info_table.clear();

	prefs.display_default_tank_infos = false;
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 0); // nothing in the open (empty) file

	// This is what ManageTankInfoDialog::done() does when the editor closes.
	save_tank_info_table();

	prefs.display_default_tank_infos = true;
	reset_tank_info_table(tank_info_table);

	// The built-in defaults must still be there, not replaced by the
	// (empty) reduced table that was displayed while the option was off.
	QVERIFY(tank_info_table.size() > 10);
	QCOMPARE(tank_info_table[0].name, std::string("10.0L"));

	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
}

QTEST_MAIN(TestQPrefEquipment)
