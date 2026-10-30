// SPDX-License-Identifier: GPL-2.0
#include "testtankinfomodel.h"

#include "core/dive.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/pref.h"
#include "core/settings/qPrefEquipment.h"
#include "qt-models/tankinfomodel.h"

#include "commands/command.h"
#include "core/divesite.h"
#if defined(MAP_SUPPORT)
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

#include <algorithm>

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>

// The list is always kept sorted alphabetically now (no manual reordering),
// so a freshly added or renamed entry doesn't necessarily land at a fixed
// row. Several tests below need to find a specific entry by name afterwards
// rather than assuming a position; this is their shared helper for that.
static int rowOf(const std::string &name)
{
	auto it = std::find_if(tank_info_table.begin(), tank_info_table.end(),
				[&name](const tank_info &ti) { return ti.name == name; });
	return it == tank_info_table.end() ? -1 : static_cast<int>(std::distance(tank_info_table.begin(), it));
}

class UnitsGuard {
public:
	UnitsGuard() : saved(prefs.units) {}
	~UnitsGuard() { prefs.units = saved; }

private:
	units saved;
};

class DefaultCylinderGuard {
public:
	DefaultCylinderGuard() : saved(qPrefEquipment::default_cylinder()) {}
	~DefaultCylinderGuard() { qPrefEquipment::set_default_cylinder(saved); }

private:
	QString saved;
};

void TestTankInfoModel::initTestCase()
{
	TestBase::initTestCase();

	// Isolated settings namespace so the persistence test below doesn't
	// read or write the real user's (or another test binary's) settings.
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestTankInfoModel");
}

// Known, deterministic starting point for every test: two metric entries
// (ml/bar representation, i.e. tank_info::ml and ::bar are non-zero) in
// tank_info_table, and an empty persisted baseline (core/equipment.cpp).
// The baseline must be reset too, and not just tank_info_table: it's a
// process-wide static that add()/remove()/resize (see
// qt-models/tankinfomodel.cpp) now mirror into, so e.g. a stray "New type"
// entry left behind by testAddInsertsEntry could otherwise leak into a
// later test's baseline-related assertions.
void TestTankInfoModel::init()
{
	tank_info_table.clear();
	tank_info_table.push_back(tank_info { "TypeA", 0, 11100, 0, 207 });
	tank_info_table.push_back(tank_info { "TypeB", 0, 10000, 0, 200 });
	set_tank_info_baseline({});
}

void TestTankInfoModel::cleanup()
{
	tank_info_table.clear();
	divelog.clear();
}

void TestTankInfoModel::testAddInsertsEntry()
{
	TankInfoModel model(nullptr);
	QCOMPARE(model.rowCount(), 2);

	// The list is kept sorted alphabetically, so the new entry doesn't
	// necessarily land on the last row - use the row add() reports instead
	// of assuming one.
	int row = model.add();

	QCOMPARE(model.rowCount(), 3);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 3);
	QCOMPARE(tank_info_table[row].ml, 0);
	QCOMPARE(tank_info_table[row].bar, 0);
	QVERIFY(!tank_info_table[row].name.empty());
}

// Regression test for the bug where add() always named a new entry
// "New type", so clicking "Add" twice produced two entries with the same
// name, breaking the model's own duplicate-name invariant (enforced by
// setData(DESCRIPTION)) and making the first entry unreachable by
// name-based lookups such as get_tank_info().
void TestTankInfoModel::testAddTwiceProducesUniqueNames()
{
	TankInfoModel model(nullptr);

	model.add();
	model.add();
	model.add();

	QCOMPARE(model.rowCount(), 5);

	// Collect the three generated placeholder names - not assuming where
	// each landed once the list was re-sorted - and check they're distinct.
	std::vector<std::string> newNames;
	for (const tank_info &ti: tank_info_table) {
		if (ti.name != "TypeA" && ti.name != "TypeB")
			newNames.push_back(ti.name);
	}
	QCOMPARE(static_cast<int>(newNames.size()), 3);
	QVERIFY(!newNames[0].empty());
	QVERIFY(!newNames[1].empty());
	QVERIFY(!newNames[2].empty());
	QVERIFY(newNames[0] != newNames[1]);
	QVERIFY(newNames[0] != newNames[2]);
	QVERIFY(newNames[1] != newNames[2]);
}

// Regression test for the bug where a brand new entry's size was silently
// discarded if the user entered it before the working pressure: cuft (the
// imperial representation update_tank_info_data() defaulted to for an
// all-zero entry) cannot represent a size without a known working pressure.
void TestTankInfoModel::testAddThenSizeBeforePressureRetainsSize()
{
	TankInfoModel model(nullptr);
	int row = model.add();

	QVERIFY(model.setData(model.index(row, TankInfoModel::ML), QStringLiteral("15")));
	QCOMPARE(tank_info_table[row].ml, 15000);

	QVERIFY(model.setData(model.index(row, TankInfoModel::BAR), QStringLiteral("200")));
	QCOMPARE(tank_info_table[row].ml, 15000);
	QCOMPARE(tank_info_table[row].bar, 200);
}

void TestTankInfoModel::testRenameDescription()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::DESCRIPTION);

	QVERIFY(model.setData(idx, QStringLiteral("Renamed")));

	QCOMPARE(tank_info_table[0].name, std::string("Renamed"));
}

void TestTankInfoModel::testRenameRejectsEmpty()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::DESCRIPTION);

	QVERIFY(!model.setData(idx, QStringLiteral("   ")));

	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
}

void TestTankInfoModel::testRenameRejectsDuplicate()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::DESCRIPTION);

	// Renaming "TypeA" to the already-existing "TypeB" must be rejected,
	// as it would make one of the two entries unreachable by name lookup.
	QVERIFY(!model.setData(idx, QStringLiteral("TypeB")));

	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
	QCOMPARE(tank_info_table[1].name, std::string("TypeB"));
}

void TestTankInfoModel::testResizeSize()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::ML);

	QVERIFY(model.setData(idx, QStringLiteral("15")));

	QCOMPARE(tank_info_table[0].ml, 15000);
	// The working pressure must be left untouched by a size-only edit.
	QCOMPARE(tank_info_table[0].bar, 207);
}

void TestTankInfoModel::testResizeRejectsInvalidInput()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::ML);

	QVERIFY(!model.setData(idx, QStringLiteral("not a number")));

	QCOMPARE(tank_info_table[0].ml, 11100);
}

void TestTankInfoModel::testResizeWorkingPressure()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::BAR);

	QVERIFY(model.setData(idx, QStringLiteral("250")));

	QCOMPARE(tank_info_table[0].bar, 250);
	// The size must be left untouched by a pressure-only edit.
	QCOMPARE(tank_info_table[0].ml, 11100);
}

void TestTankInfoModel::testResizeUnchangedWorkingPressureIsNoop()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::BAR);

	QString displayed = model.data(idx, Qt::EditRole).toString();
	QVERIFY(!model.setData(idx, displayed));

	QCOMPARE(tank_info_table[0].ml, 11100);
	QCOMPARE(tank_info_table[0].bar, 207);
}

// Regression test for the fix that made setData() bail out of the ML branch
// without reparsing when the submitted text is identical to what's
// currently displayed. The view's default item delegate resubmits the
// current text whenever an edit is committed, even if the user never typed
// anything (e.g. opening the cell and pressing Enter/Tab), so this must be
// a true no-op: no reparse, and no change to tank_info_table.
void TestTankInfoModel::testResizeUnchangedValueIsNoop()
{
	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::ML);

	QString displayed = model.data(idx, Qt::EditRole).toString();
	QVERIFY(!model.setData(idx, displayed));

	QCOMPARE(tank_info_table[0].ml, 11100);
	QCOMPARE(tank_info_table[0].bar, 207);
}

// Same fix, but for the specific scenario that motivated it: with "cuft"
// display chosen, get_cylinder_size_string() rounds to as little as 0
// decimals, so round-tripping an unmodified value back through
// string_to_volume() can land on a different ml value than what's actually
// stored (11100 ml / 207 bar displays as "80cuft", which reparses to 11086
// ml, a silent 14 ml drift, if it isn't recognised as unchanged first).
void TestTankInfoModel::testResizeUnchangedCuftValueAvoidsRoundingDrift()
{
	UnitsGuard unitsGuard;
	prefs.units.volume = units::CUFT;

	TankInfoModel model(nullptr);
	QModelIndex idx = model.index(0, TankInfoModel::ML);

	QString displayed = model.data(idx, Qt::EditRole).toString();
	QVERIFY(!model.setData(idx, displayed));

	QCOMPARE(tank_info_table[0].ml, 11100);
	QCOMPARE(tank_info_table[0].bar, 207);
}

void TestTankInfoModel::testRemoveEntry()
{
	set_tank_info_baseline({ tank_info { "TypeA", 0, 11100, 0, 207 } });
	TankInfoModel model(nullptr);

	model.remove(0);

	QCOMPARE(model.rowCount(), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeB"));
}

void TestTankInfoModel::testRemoveDefaultEntryClearsDefaultCylinder()
{
	DefaultCylinderGuard defaultCylinderGuard;
	set_tank_info_baseline({ tank_info { "TypeA", 0, 11100, 0, 207 } });
	qPrefEquipment::set_default_cylinder(QStringLiteral("TypeA"));
	TankInfoModel model(nullptr);

	model.remove(0);

	QCOMPARE(qPrefEquipment::default_cylinder(), QString());
}

void TestTankInfoModel::testRemoveOutOfRangeIsNoop()
{
	TankInfoModel model(nullptr);

	model.remove(-1);
	model.remove(100);

	QCOMPARE(model.rowCount(), 2);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
	QCOMPARE(tank_info_table[1].name, std::string("TypeB"));
}

// Regression tests for the removal of manual reordering ("Move up"/"Move
// down"), replaced by a stable, automatic sort: the list must always be
// shown alphabetically by name, including right after construction, after
// add(), and after a rename that would otherwise leave it out of order.

void TestTankInfoModel::testListStaysSortedAfterAdd()
{
	tank_info_table.clear();
	tank_info_table.push_back(tank_info { "Charlie", 0, 1, 0, 1 });
	tank_info_table.push_back(tank_info { "alpha", 0, 1, 0, 1 });
	TankInfoModel model(nullptr); // sorts on construction

	QCOMPARE(tank_info_table[0].name, std::string("alpha"));
	QCOMPARE(tank_info_table[1].name, std::string("Charlie"));

	// The new "New type" entry sorts after both ('N' > 'C' > 'a'
	// case-insensitively), so it must land at the end, not wherever
	// add() used to simply append.
	int row = model.add();

	QCOMPARE(row, 2);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 3);
	QCOMPARE(tank_info_table[0].name, std::string("alpha"));
	QCOMPARE(tank_info_table[1].name, std::string("Charlie"));
	QCOMPARE(tank_info_table[2].name, tank_info_table[row].name);
}

void TestTankInfoModel::testRenameResortsListWhenOrderChanges()
{
	tank_info_table.clear();
	tank_info_table.push_back(tank_info { "alpha", 0, 11100, 0, 207 });
	tank_info_table.push_back(tank_info { "Charlie", 0, 10000, 0, 200 });
	TankInfoModel model(nullptr);
	QCOMPARE(tank_info_table[0].name, std::string("alpha"));
	QCOMPARE(tank_info_table[1].name, std::string("Charlie"));

	// Renaming "Charlie" (row 1) to "Aardvark" must move it before "alpha",
	// not leave it sitting in its old row out of order.
	QVERIFY(model.setData(model.index(1, TankInfoModel::DESCRIPTION), QStringLiteral("Aardvark")));

	QCOMPARE(tank_info_table[0].name, std::string("Aardvark"));
	QCOMPARE(tank_info_table[1].name, std::string("alpha"));
}

void TestTankInfoModel::testSortIsCaseInsensitive()
{
	tank_info_table.clear();
	tank_info_table.push_back(tank_info { "Cherry", 0, 1, 0, 1 });
	tank_info_table.push_back(tank_info { "banana", 0, 1, 0, 1 });

	// Plain byte/ASCII ("case-sensitive") ordering would put "Cherry"
	// first ('C' is 67, 'b' is 98); case-insensitive alphabetical order
	// must put "banana" first instead.
	TankInfoModel model(nullptr); // sorts on construction

	QCOMPARE(tank_info_table[0].name, std::string("banana"));
	QCOMPARE(tank_info_table[1].name, std::string("Cherry"));
}

// Verifies the full user-facing flow: edits made through TankInfoModel (as
// driven by ManageTankInfoDialog) are persisted when the dialog closes
// (save_tank_info_table(), called directly here rather than through the
// dialog itself to keep this a qt-models-level test) and are rebuilt
// correctly the next time a dive log is opened after an application
// restart (simulated by clearing tank_info_table and divelog, then asking
// the preference layer to reload from disk exactly as qPref::load() does
// at startup).
void TestTankInfoModel::testEditsPersistAcrossRestart()
{
	// Unlike most other tests in this file, TypeA/TypeB need to be baseline
	// members (not just tank_info_table entries) here: only the baseline is
	// what save_tank_info_table() persists (see core/equipment.h), so a
	// "survives a restart" test needs a baseline fixture to be meaningful.
	set_tank_info_baseline({
		tank_info { "TypeA", 0, 11100, 0, 207 },
		tank_info { "TypeB", 0, 10000, 0, 200 },
	});

	TankInfoModel model(nullptr);

	QVERIFY(model.setData(model.index(0, TankInfoModel::DESCRIPTION), QStringLiteral("TypeA Renamed")));
	model.remove(1); // delete TypeB
	// The list is kept sorted alphabetically, so the new entry doesn't
	// necessarily land on row 1 - use the row add() reports instead.
	int newRow = model.add();
	QVERIFY(model.setData(model.index(newRow, TankInfoModel::DESCRIPTION), QStringLiteral("TypeC")));

	save_tank_info_table();

	tank_info_table.clear();
	divelog.clear();
	prefs.display_default_tank_infos = true;
	qPrefEquipment::instance()->load();
	reset_tank_info_table(tank_info_table);

	QCOMPARE(static_cast<int>(tank_info_table.size()), 2);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));
	QCOMPARE(tank_info_table[0].ml, 11100);
	QCOMPARE(tank_info_table[1].name, std::string("TypeC"));
}

// Verifies the fix for the usability bug where renaming a global cylinder
// type through the editor left any dive that already used the old name
// unchanged: closing and reopening Preferences (which rebuilds
// tank_info_table from tank_info_table's defaults plus whatever names are
// still referenced by divelog.dives) would then resurrect the old name as a
// spurious duplicate. TankInfoModel::setData() now cascades a rename into
// the currently loaded dive log via Command::renameCylinderType(), stubbed
// below to perform the real cascade (via the production
// find_cylinders_with_type_description() helper) without requiring the
// undo-stack machinery that this test binary deliberately doesn't link.
void TestTankInfoModel::testRenameCascadesToDiveCylinders()
{
	auto d = std::make_unique<dive>();
	cylinder_t cyl;
	cyl.type.description = "TypeA";
	cyl.type.size.mliter = 11100;
	cyl.type.workingpressure.mbar = 207000;
	d->cylinders.push_back(cyl);
	dive *dPtr = divelog.dives.register_dive(std::move(d));

	TankInfoModel model(nullptr);
	QVERIFY(model.setData(model.index(0, TankInfoModel::DESCRIPTION), QStringLiteral("TypeA Renamed")));

	// The dive's cylinder must now reference the new name...
	QCOMPARE(dPtr->cylinders[0].type.description, std::string("TypeA Renamed"));

	// ...so a subsequent merge of the dive log's cylinder types into
	// tank_info_table (as done every time Preferences is applied, or a file
	// is loaded) must not resurrect "TypeA" as a spurious duplicate.
	prefs.display_default_tank_infos = false;
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));
}

// Regression tests for the fix that stopped the editor's add/resize/remove
// actions from losing the persisted baseline (core/equipment.cpp) or
// silently promoting a dive-log-only type into it. tank_info_table is only
// the current session's runtime display/picker view (baseline plus
// whatever the open file uses - see reset_tank_info_table()); the baseline
// is what save_tank_info_table() actually persists, and the editor must
// mirror edits into it directly, with skip-if-absent semantics for
// anything that was never a baseline member.

// add() always creates a library-worthy entry: it must land in both lists.
void TestTankInfoModel::testAddMirrorsIntoBaseline()
{
	TankInfoModel model(nullptr);

	int row = model.add();

	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QCOMPARE(get_tank_info_baseline()[0].name, tank_info_table[row].name);
}

void TestTankInfoModel::testRemoveMirrorsIntoBaselineOnlyIfPresent()
{
	TankInfoModel model(nullptr);
	int addedRow = model.add(); // also added to the baseline, see testAddMirrorsIntoBaseline()
	std::string addedName = tank_info_table[addedRow].name;
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);

	// Removing "TypeA" (never a baseline member - see init()) must leave
	// it in the runtime catalogue and leave the baseline untouched...
	model.remove(rowOf("TypeA"));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QVERIFY(rowOf("TypeA") >= 0);

	// ...but removing the entry that add() did put in the baseline must
	// remove it from there too.
	model.remove(rowOf(addedName));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);
}

void TestTankInfoModel::testResizeMirrorsIntoBaselineOnlyIfPresent()
{
	TankInfoModel model(nullptr);
	int addedRow = model.add(); // "New type", 0/0/0/0, also added to the baseline
	std::string addedName = tank_info_table[addedRow].name;

	// Resizing "TypeA" (never a baseline member) must not create a
	// baseline entry for it.
	QVERIFY(model.setData(model.index(rowOf("TypeA"), TankInfoModel::ML), QStringLiteral("15")));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QCOMPARE(get_tank_info_baseline()[0].name, addedName);

	// Resizing the entry that IS a baseline member must update it there too.
	// (Resizing doesn't change the name, so addedRow's row stays valid
	// across both setData() calls below.)
	int newRow = rowOf(addedName);
	QVERIFY(model.setData(model.index(newRow, TankInfoModel::ML), QStringLiteral("15")));
	QVERIFY(model.setData(model.index(newRow, TankInfoModel::BAR), QStringLiteral("200")));
	tank_info *baselineInfo = get_tank_info(get_tank_info_baseline(), addedName);
	QVERIFY(baselineInfo != nullptr);
	QCOMPARE(baselineInfo->ml, 15000);
	QCOMPARE(baselineInfo->bar, 200);
}

// Acceptance scenario: a type that only appears in the editor because the
// currently open dive log uses it (and was never added through "Add", so
// it's never been a baseline member) must not be promoted into the
// persisted baseline merely by being displayed or resized. It must also
// remain in the runtime catalogue when removal is requested: loaded dives
// still reference the type, and the global library editor cannot remove a
// file-only entry.
void TestTankInfoModel::testFileOnlyTypeNotPromotedToBaseline()
{
	auto d = std::make_unique<dive>();
	cylinder_t cyl;
	cyl.type.description = "FileOnlyType";
	cyl.type.size.mliter = 11100;
	cyl.type.workingpressure.mbar = 207000;
	d->cylinders.push_back(cyl);
	divelog.dives.register_dive(std::move(d));

	prefs.display_default_tank_infos = false;
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("FileOnlyType"));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);

	TankInfoModel model(nullptr);
	QVERIFY(model.setData(model.index(0, TankInfoModel::ML), QStringLiteral("15")));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);

	model.remove(0);
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);
	QCOMPARE(model.rowCount(), 1);
	QCOMPARE(tank_info_table[0].name, std::string("FileOnlyType"));
}

// Regression test for the fix that made Command::RenameCylinderType the
// sole owner of a rename across tank_info_table AND the persisted baseline.
// Previously, only TankInfoModel::setData() renamed tank_info_table
// (directly, outside of any command), while the command only cascaded the
// per-dive cylinder descriptions; undoing left tank_info_table (and, after
// the baseline-persistence fix, the baseline) stuck on the new name, so the
// next reset_tank_info_table() resurrected the old name as a spurious
// duplicate, and a subsequent redo left a stray new-name row behind.
// Exercises the stubbed Command::renameCylinderType() directly to simulate
// a redo/undo/redo cycle, since the real undo-stack machinery isn't linked
// into this test binary (see the file-level comment above); the stub
// performs the same name-based tank_info_table/baseline mutations as the
// real RenameCylinderType::redo()/undo() now do.
void TestTankInfoModel::testRenameUndoRedoKeepsTableAndBaselineInSync()
{
	set_tank_info_baseline({ tank_info { "TypeA", 0, 11100, 0, 207 } });

	// redo: TypeA -> TypeA Renamed
	Command::renameCylinderType("TypeA", "TypeA Renamed");
	QCOMPARE(static_cast<int>(tank_info_table.size()), 2);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QCOMPARE(get_tank_info_baseline()[0].name, std::string("TypeA Renamed"));

	// undo: TypeA Renamed -> TypeA, with no duplicate left behind in either list
	Command::renameCylinderType("TypeA Renamed", "TypeA");
	QCOMPARE(static_cast<int>(tank_info_table.size()), 2);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
	QVERIFY(get_tank_info(tank_info_table, "TypeA Renamed") == nullptr);
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QCOMPARE(get_tank_info_baseline()[0].name, std::string("TypeA"));
	QVERIFY(get_tank_info(get_tank_info_baseline(), "TypeA Renamed") == nullptr);

	// redo again: no stray row left behind in either list either
	Command::renameCylinderType("TypeA", "TypeA Renamed");
	QCOMPARE(static_cast<int>(tank_info_table.size()), 2);
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));
	QCOMPARE(get_tank_info_baseline()[0].name, std::string("TypeA Renamed"));
}

// Acceptance scenario: renaming a type that is only present because the
// open dive log uses it (never a baseline member) must still cascade
// correctly into the dive data and tank_info_table, in both the redo and
// undo direction, while leaving the (empty) baseline untouched throughout.
void TestTankInfoModel::testRenameFileOnlyTypeLeavesBaselineUntouched()
{
	auto d = std::make_unique<dive>();
	cylinder_t cyl;
	cyl.type.description = "TypeA";
	cyl.type.size.mliter = 11100;
	cyl.type.workingpressure.mbar = 207000;
	d->cylinders.push_back(cyl);
	dive *dPtr = divelog.dives.register_dive(std::move(d));

	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);

	TankInfoModel model(nullptr);
	QVERIFY(model.setData(model.index(0, TankInfoModel::DESCRIPTION), QStringLiteral("TypeA Renamed")));
	QCOMPARE(dPtr->cylinders[0].type.description, std::string("TypeA Renamed"));
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);

	// undo direction
	Command::renameCylinderType("TypeA Renamed", "TypeA");
	QCOMPARE(dPtr->cylinders[0].type.description, std::string("TypeA"));
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 0);
}

// Regression test for the fix that made a baseline rename re-persist
// immediately from within redo()/undo(), instead of only ever being saved
// by ManageTankInfoDialog::done() when the editor closes. Before this fix,
// an Edit > Undo/Redo from the main window (long after the dialog closed)
// only updated the in-memory baseline, leaving QSettings on the stale name;
// a later restart would reload that stale name and merge the dive log's
// (reverted) cylinder types back in, reintroducing the exact stray-duplicate
// scenario this feature exists to prevent. Checks that QSettings reflects
// each rename immediately, without ever calling save_tank_info_table()
// directly or reopening the dialog - only through the stubbed
// Command::renameCylinderType(), which now mirrors the real command's
// re-persist-on-baseline-rename behaviour (see the stub's comment below).
void TestTankInfoModel::testRenameUndoPersistsRevertedNameImmediately()
{
	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
	prefs.display_default_tank_infos = true;
	set_tank_info_baseline({ tank_info { "TypeA", 0, 11100, 0, 207 } });

	// redo: rename "TypeA" -> "TypeA Renamed", as TankInfoModel::setData()
	// would trigger by pushing the command.
	Command::renameCylinderType("TypeA", "TypeA Renamed");

	// Simulate an application restart without ever reopening the editor:
	// QSettings must already reflect the rename.
	tank_info_table.clear();
	divelog.clear();
	qPrefEquipment::instance()->load();
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA Renamed"));

	// undo: Edit > Undo from the main window, well after the "restart" above.
	Command::renameCylinderType("TypeA Renamed", "TypeA");

	// QSettings must reflect the reverted name immediately too.
	tank_info_table.clear();
	divelog.clear();
	qPrefEquipment::instance()->load();
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));

	QSettings().remove(QStringLiteral("Equipment/TankTypes"));
}

// Regression test for the fix that made the "New type" placeholder name
// unique against the baseline as well as tank_info_table. With "Show
// default cylinder sizes" off, reset_tank_info_table() (as run on a file
// reload) builds tank_info_table only from the dive log's own cylinder
// types, so a type added to the baseline (but not used by any dive) drops
// out of tank_info_table entirely. A tank_info_table-only uniqueness check
// would then let a second "Add" reuse the same placeholder name, producing
// two same-named entries in the persisted baseline - only the first ever
// reachable by get_tank_info()/remove()'s own baseline lookups.
void TestTankInfoModel::testAddAcrossReloadNeverDuplicatesBaselineName()
{
	prefs.display_default_tank_infos = false;
	tank_info_table.clear(); // no dive-log-only types in this scenario
	TankInfoModel model(nullptr);

	model.add();
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);
	std::string firstName = get_tank_info_baseline()[0].name;

	// Simulate a file reload: with defaults off and no dive using it, the
	// added entry drops out of tank_info_table, even though it's still in
	// the baseline.
	divelog.clear();
	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 0);
	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 1);

	model.add();

	QCOMPARE(static_cast<int>(get_tank_info_baseline().size()), 2);
	QVERIFY(get_tank_info_baseline()[1].name != firstName);
}

// Regression test for the fix that made the DESCRIPTION duplicate-rejection
// check in setData() consider the baseline as well as tank_info_table. With
// "Show default cylinder sizes" off, a baseline member not used by any
// currently open dive is invisible in tank_info_table, so a
// tank_info_table-only check would let an open file's type be renamed to
// that baseline-only name, producing two same-named entries in the
// persisted baseline once the rename mirrors into it.
void TestTankInfoModel::testRenameRejectsNameOnlyInBaseline()
{
	set_tank_info_baseline({
		tank_info { "TypeA", 0, 11100, 0, 207 },
		tank_info { "TypeB", 0, 10000, 0, 200 },
	});
	prefs.display_default_tank_infos = false;

	auto d = std::make_unique<dive>();
	cylinder_t cyl;
	cyl.type.description = "TypeA";
	cyl.type.size.mliter = 11100;
	cyl.type.workingpressure.mbar = 207000;
	d->cylinders.push_back(cyl);
	divelog.dives.register_dive(std::move(d));

	reset_tank_info_table(tank_info_table);
	QCOMPARE(static_cast<int>(tank_info_table.size()), 1);
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));

	TankInfoModel model(nullptr);
	// "TypeB" isn't visible in tank_info_table right now (defaults are off
	// and no open dive uses it), but it IS already in the baseline, so the
	// rename must still be rejected.
	QVERIFY(!model.setData(model.index(0, TankInfoModel::DESCRIPTION), QStringLiteral("TypeB")));
	QCOMPARE(tank_info_table[0].name, std::string("TypeA"));
}

// Regression test for the fix that let Preferences' "default cylinder"
// combo follow a renamed type by identity instead of by a name string
// captured before the "Manage cylinder types" dialog opened (which goes
// stale the instant that exact entry is renamed, silently losing the
// selection to the combo's first entry - see
// ManageTankInfoDialog::onCylinderTypeRenamed() and trackedName() in
// desktop-widgets/preferences/managetankinfodialog.cpp, which consume this
// signal). The full ManageTankInfoDialog/PreferencesEquipment widget
// interaction isn't testable from this binary: both classes need
// ui_managetankinfodialog.h/ui_preferences_equipment.h, generated by
// subsurface_generated_ui, which this binary deliberately doesn't link (see
// the DesktopExecutable guard this test is itself built under, and the
// fix for TestTankInfoModel unconditionally needing desktop-widgets headers
// it fixed). This instead covers the part of the fix that IS reachable from
// here: that setData() emits the signal exactly when (and with exactly the
// names with which) a rename is actually applied.
void TestTankInfoModel::testRenameEmitsCylinderTypeRenamedSignal()
{
	TankInfoModel model(nullptr);
	QSignalSpy spy(&model, &TankInfoModel::cylinderTypeRenamed);

	QVERIFY(model.setData(model.index(0, TankInfoModel::DESCRIPTION), QStringLiteral("TypeA Renamed")));

	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("TypeA"));
	QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("TypeA Renamed"));

	// Rejected edits (here: an empty name) must not emit the signal.
	QVERIFY(!model.setData(model.index(1, TankInfoModel::DESCRIPTION), QStringLiteral("   ")));
	QCOMPARE(spy.count(), 1);
}

// Stubs for symbols referenced (but never actually exercised) by other
// desktop qt-models source files that end up linked into this test binary
// alongside TankInfoModel, because they all share subsurface_models_desktop's
// combined moc compilation unit. See testdiveplannermodel.cpp for the same
// pattern.
namespace Command {

void addDive(std::unique_ptr<dive>, bool, bool) {}
bool importDives(struct divelog *, int, const QString &) { return false; }
void replanDive(dive *) {}
int editCylinder(int, cylinder_t, EditCylinderType, bool) { return 0; }
void editSensors(int, int, int) {}
void editDiveSiteName(dive_site *, const QString &) {}
void editDiveSiteDescription(dive_site *, const QString &) {}
void removePictures(const std::vector<PictureListForDeletion> &) {}
int editWeight(int, weightsystem_t, bool) { return 0; }

// The real implementation (commands/command_edit.cpp) additionally wraps
// this in an undo command; that machinery isn't linked into this test
// binary (see the file-level comment above), so this stub applies the same
// underlying core/equipment.cpp lookups directly that RenameCylinderType's
// redo()/undo() now perform: the per-dive cascade, a name-based rename of
// the matching tank_info_table and get_tank_info_baseline() entries, if any
// (skip-if-absent in both cases), and an immediate re-persist whenever the
// baseline entry itself was renamed (not just tank_info_table), so a later
// undo/redo doesn't leave QSettings stuck on a stale name. The real command
// is immune to the caller passing it a live alias of the very object it's
// about to rename (e.g. a tank_info_table entry's own .name) because its
// oldName/newName are independent member variables, copied once at
// construction time, before anything is mutated; take the same defensive
// local copy here so this stub behaves the same way regardless of what the
// caller passes.
void renameCylinderType(const std::string &oldNameIn, const std::string &newName)
{
	std::string oldName = oldNameIn;
	for (auto &[d, idx]: find_cylinders_with_type_description(oldName))
		d->cylinders[idx].type.description = newName;
	if (tank_info *info = get_tank_info(tank_info_table, oldName))
		info->name = newName;
	if (tank_info *info = get_tank_info(get_tank_info_baseline(), oldName)) {
		info->name = newName;
		save_tank_info_table();
	}
}

} // namespace Command

#if defined(MAP_SUPPORT)
MapWidget *MapWidget::m_instance = nullptr;

MapWidget *MapWidget::instance()
{
	return m_instance;
}

bool MapWidget::editMode() const
{
	return false;
}

void MapWidget::reload()
{
}

void MapWidget::setSelected(std::vector<dive_site *>)
{
}

MainWindow *MainWindow::instance()
{
	return nullptr;
}
#endif

QTEST_GUILESS_MAIN(TestTankInfoModel)
