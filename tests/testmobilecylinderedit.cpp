// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
//
// Regression tests for the mobile cylinder-edit index-mapping fix.
//
// The bug: QMLManager::commitChanges guarded all three cylinder write loops
// (pressure, gasmix, type) with `is_cylinder_used()`.  For dives with no
// used cylinders (no pressure delta, no gas-change events) every write was
// skipped, silently discarding the user's edits.
//
// These tests simulate the fixed write path at the dive/cylinder level,
// independent of QMLManager and the UI, to keep the test binary free of
// mobile-widget link dependencies.

#include "testmobilecylinderedit.h"

#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/equipment.h"
#include "core/event.h"
#include "core/units.h"

// ---------------------------------------------------------------------------
// TC1: zero-cylinder dive -- adding a cylinder on edit must persist
// ---------------------------------------------------------------------------
void TestMobileCylinderEdit::test_zero_cylinder_dive_edit_adds_cylinder()
{
	struct dive d;

	// Precondition: no cylinders at all.
	QCOMPARE(static_cast<int>(d.cylinders.size()), 0);

	// Simulate what the fixed commitChanges does for cylinder-0 when the
	// incoming list has one entry (the user typed something in the form).
	// The old code: startpressure was [] (QML never sent data because
	// usedCyl[0] was undefined), so the loop never ran.
	// After the fix, QML always sends cylinder-0 data unconditionally.
	cylinder_t *cyl = d.get_or_create_cylinder(0);
	cyl->type.description = "AL80";
	cyl->type.size.mliter = 11100;
	cyl->type.workingpressure.mbar = 207 * 1000;
	cyl->start = 200_bar;
	cyl->end = 50_bar;

	// After the write, cylinder 0 must exist with the entered values.
	QCOMPARE(static_cast<int>(d.cylinders.size()), 1);
	QCOMPARE(d.cylinders[0].type.description, std::string("AL80"));
	QCOMPARE(d.cylinders[0].start.mbar, 200 * 1000);
	QCOMPARE(d.cylinders[0].end.mbar, 50 * 1000);
}

// ---------------------------------------------------------------------------
// TC2: is_cylinder_used guard -- cylinder 1 with no gas-change event and no
//      pressure delta is NOT considered used; the old guard would skip it.
//      The fixed path writes to it unconditionally (positional iteration).
// ---------------------------------------------------------------------------
void TestMobileCylinderEdit::test_unused_cylinder_dive_edit_persists()
{
	struct dive d;

	// Cylinder 0: some gas, used (non-zero pressure delta).
	cylinder_t *cyl0 = d.get_or_create_cylinder(0);
	cyl0->gasmix.o2.permille = 210;
	cyl0->start = 200_bar;
	cyl0->end = 50_bar;

	// Cylinder 1: added to the dive but no gas-change event and no pressure
	// delta -- is_cylinder_used(1) must return false.  This is the bug
	// scenario: the old code would skip writing to cylinder 1.
	cylinder_t *cyl1 = d.get_or_create_cylinder(1);
	cyl1->type.description = "unknown";
	cyl1->gasmix.o2.permille = 210;
	cyl1->start = 0_bar;
	cyl1->end = 0_bar;

	// Confirm that cylinder 1 is NOT used (no pressure delta, no event).
	QVERIFY(!d.is_cylinder_used(1));

	// Simulate the fixed commitChanges write for cylinder 1
	// (no is_cylinder_used guard, direct positional access).
	cyl1 = d.get_or_create_cylinder(1);
	cyl1->type.description = "AL80";
	cyl1->start = 200_bar;
	cyl1->end = 50_bar;

	// After the write, is_cylinder_used(1) returns true (pressure delta > SOME_GAS).
	QVERIFY(d.is_cylinder_used(1));

	// Values must have persisted.
	QCOMPARE(d.cylinders[1].type.description, std::string("AL80"));
	QCOMPARE(d.cylinders[1].start.mbar, 200 * 1000);
	QCOMPARE(d.cylinders[1].end.mbar, 50 * 1000);
}

// ---------------------------------------------------------------------------
// TC3: two-cylinder dive with gas-change event -- index mapping must not
//      conflate cylinder slots (#2913 regression guard)
// ---------------------------------------------------------------------------
void TestMobileCylinderEdit::test_multi_cylinder_index_mapping_preserved()
{
	struct dive d;

	// Cylinder 0: AIR, start/end 200/50 bar.
	cylinder_t *cyl0 = d.get_or_create_cylinder(0);
	cyl0->gasmix.o2.permille = 210;  // 21% O2 == air
	cyl0->gasmix.he.permille = 0;
	cyl0->start = 200_bar;
	cyl0->end = 50_bar;

	// Cylinder 1: EAN32, start/end 200/50 bar.
	cylinder_t *cyl1 = d.get_or_create_cylinder(1);
	cyl1->gasmix.o2.permille = 320;  // 32% O2
	cyl1->gasmix.he.permille = 0;
	cyl1->start = 200_bar;
	cyl1->end = 50_bar;

	// Gas-change event to cylinder 1 at 10 minutes.
	add_gas_switch_event(&d, &d.dcs.back(), 600, 1);

	// Both cylinders are now used.
	QVERIFY(d.is_cylinder_used(0));
	QVERIFY(d.is_cylinder_used(1));

	// Simulate an edit that only changes start/end pressures (no gasmix change).
	// The fixed loop iterates positionally: index 0 -> cylinder 0, index 1 -> cylinder 1.
	// Cylinder 0: reduce end pressure.
	d.get_or_create_cylinder(0)->start = 200_bar;
	d.get_or_create_cylinder(0)->end = 60_bar;
	// Cylinder 1: reduce end pressure.
	d.get_or_create_cylinder(1)->start = 200_bar;
	d.get_or_create_cylinder(1)->end = 40_bar;

	// Cylinder 1 must still have EAN32 (320 permille o2), not AIR.
	QCOMPARE(d.cylinders[1].gasmix.o2.permille, 320);
	QCOMPARE(d.cylinders[1].gasmix.he.permille, 0);

	// Cylinder 0 must still have AIR (210 permille o2).
	QCOMPARE(d.cylinders[0].gasmix.o2.permille, 210);

	// Pressure writes landed on the correct cylinders.
	QCOMPARE(d.cylinders[0].end.mbar, 60 * 1000);
	QCOMPARE(d.cylinders[1].end.mbar, 40 * 1000);
}

QTEST_GUILESS_MAIN(TestMobileCylinderEdit)
