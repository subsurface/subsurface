// SPDX-License-Identifier: GPL-2.0
#include "testdiveplannermodel.h"
#include "qt-models/diveplannermodel.h"
#include "backend-shared/plannershared.h"
#include "core/subsurfacestartup.h"
#include "commands/command.h"
#include "core/divelog.h"
#include "core/dive.h"
#include "core/equipment.h"
#include "core/sample.h"
#include "core/pref.h"
#include "core/string-format.h"
#include "core/units.h"
#include <QSignalSpy>
#include <algorithm>
#include <memory>
#include <vector>

#ifdef MAP_SUPPORT
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/mainwindow.h"
#endif

void TestDivePlannerModel::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestDivePlannerModel");
}

void TestDivePlannerModel::testEmptyModelDataAccess()
{
	// Test that accessing data on an empty model doesn't crash
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	
	model->resetPlanState();
	
	// Try to access data - should return invalid QVariant, not crash
	QModelIndex invalidIndex = model->index(0, 0);
	QVariant result = model->data(invalidIndex, Qt::DisplayRole);
	QVERIFY(!result.isValid());
}

void TestDivePlannerModel::testEmptyModelEmitDataChanged()
{
	// Test that emitDataChanged on an empty model doesn't crash or emit invalid signals
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	
	model->resetPlanState();
	
	// Set up signal spy to verify no dataChanged signal with invalid range is emitted
	QSignalSpy spy(model, &DivePlannerPointsModel::dataChanged);
	
	// This should not crash and should not emit dataChanged for an empty model
	model->emitDataChanged();
	
	// Verify no signal was emitted (model is empty)
	QCOMPARE(spy.count(), 0);
}

void TestDivePlannerModel::testInvalidCylinderIndex()
{
	// This test verifies that the model handles invalid cylinder indices gracefully
	// Note: This is testing the internal validation, not creating an actual invalid state
	// The fix ensures cylinder access is checked before accessing cylinders
	
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	model->resetPlanState();
	
	// When model is in NOTHING mode, data() should return invalid for any index
	QModelIndex testIndex = model->index(0, DivePlannerPointsModel::GAS);
	QVariant result = model->data(testIndex, Qt::DisplayRole);
	QVERIFY(!result.isValid());
}

void TestDivePlannerModel::testInvalidRowIndex()
{
	// Test that accessing an invalid row index doesn't crash
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	model->resetPlanState();
	
	// Try various invalid indices
	QModelIndex invalidIndex1 = model->index(-1, 0);
	QVariant result1 = model->data(invalidIndex1, Qt::DisplayRole);
	QVERIFY(!result1.isValid());
	
	QModelIndex invalidIndex2 = model->index(9999, 0);
	QVariant result2 = model->data(invalidIndex2, Qt::DisplayRole);
	QVERIFY(!result2.isValid());
	
	// Index that looks valid but is out of bounds for empty model
	QModelIndex invalidIndex3 = model->index(0, 0);
	QVariant result3 = model->data(invalidIndex3, Qt::DisplayRole);
	QVERIFY(!result3.isValid());
}

void TestDivePlannerModel::testNothingModeDataAccess()
{
	// Test that when mode is NOTHING, data access returns invalid QVariant
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	model->resetPlanState();
	
	// Test all column types
	for (int col = 0; col < model->columnCount(); ++col) {
		QModelIndex idx = model->index(0, col);
		QVariant result = model->data(idx, Qt::DisplayRole);
		QVERIFY(!result.isValid());
	}
	
	// Test flags() as well
	QModelIndex idx = model->index(0, 0);
	Qt::ItemFlags flags = model->flags(idx);
	QCOMPARE(flags, Qt::NoItemFlags);
}

void TestDivePlannerModel::testSurfaceAirCylinderDataAccess()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);

	QVERIFY(model->rowCount() > 1);
	int surfaceAirCylinder = static_cast<int>(plannedDive.cylinders.size());
	QModelIndex gasIndex = model->index(1, DivePlannerPointsModel::GAS);
	model->gasChange(gasIndex, surfaceAirCylinder);

	QVariant gas = model->data(gasIndex, Qt::DisplayRole);
	QVERIFY(gas.isValid());
	QCOMPARE(gas.toString(), QStringLiteral("Air"));

	model->resetPlanState();
}

// AI-generated (Claude)
void TestDivePlannerModel::testImportMissingCylinderDepth()
{
	prefs = default_prefs;
	dive importedDive;
	cylinder_t cylinder;
	cylinder.gasmix.o2 = 50_percent;
	importedDive.cylinders.push_back(cylinder);
	depth_t expected = calculate_deco_switch_depth(&importedDive, cylinder.gasmix);

	normalize_imported_cylinder_depths(&importedDive);

	QVERIFY(expected.mm != 0);
	QCOMPARE(importedDive.cylinders[0].depth.mm, expected.mm);
	prefs = default_prefs;
}

// AI-generated (Claude)
void TestDivePlannerModel::testImportedCylinderDepthPreserved()
{
	prefs = default_prefs;
	dive importedDive;
	cylinder_t cylinder;
	cylinder.gasmix.o2 = 50_percent;
	cylinder.depth = 12_m;
	importedDive.cylinders.push_back(cylinder);

	normalize_imported_cylinder_depths(&importedDive);

	QCOMPARE(importedDive.cylinders[0].depth.mm, 12000);
	prefs = default_prefs;
}

// AI-generated (Claude)
void TestDivePlannerModel::testCylinderDepthInput()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;
	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);

	CylindersModel *cylinders = model->cylindersModel();
	QModelIndex depthIndex = cylinders->index(0, CylindersModel::DEPTH);
	cylinder_t *cylinder = plannedDive.get_cylinder(0);
	depth_t calculatedDepth = calculate_deco_switch_depth(&plannedDive, cylinder->gasmix);

	QVERIFY(cylinders->setData(depthIndex, QStringLiteral("12 m")));
	QCOMPARE(cylinder->depth.mm, 12000);

	QVERIFY(cylinders->setData(depthIndex, QString()));
	QCOMPARE(cylinder->depth.mm, calculatedDepth.mm);

	QVERIFY(cylinders->setData(depthIndex, QStringLiteral("12 m")));
	QVERIFY(cylinders->setData(depthIndex, QStringLiteral("  \t")));
	QCOMPARE(cylinder->depth.mm, calculatedDepth.mm);

	QVERIFY(cylinders->setData(depthIndex, QStringLiteral("12 m")));
	QVERIFY(cylinders->setData(depthIndex, QStringLiteral("0")));
	QCOMPARE(cylinder->depth.mm, 0);
	QCOMPARE(cylinders->data(depthIndex, Qt::DisplayRole).toString(), get_depth_string(0_m, true));

	model->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
void TestDivePlannerModel::testDecoSwitchDepthValidation()
{
	DivePlannerPointsModel *planner = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	planner->setPlanMode(DivePlannerPointsModel::PLAN);
	planner->createSimpleDive(&plannedDive);
	CylindersModel *model = planner->cylindersModel();
	QModelIndex depthIndex = model->index(0, CylindersModel::DEPTH);
	cylinder_t *cylinder = plannedDive.get_cylinder(0);
	QVERIFY(cylinder);
	depth_t calculatedDepth = calculate_deco_switch_depth(&plannedDive, cylinder->gasmix);
	QString zeroDepth = get_depth_string(0_m, true);

	cylinder->depth = 15_m;
	QVERIFY(model->setData(depthIndex, QString()));
	QCOMPARE(cylinder->depth.mm, calculatedDepth.mm);

	cylinder->depth = 15_m;
	QVERIFY(model->setData(depthIndex, QStringLiteral("0")));
	QCOMPARE(cylinder->depth.mm, 0);
	QCOMPARE(model->data(depthIndex, Qt::DisplayRole).toString(), zeroDepth);

	QVERIFY(model->setData(depthIndex, QStringLiteral("12,5 m")));
	QCOMPARE(cylinder->depth.mm, 12500);
	QVERIFY(model->setData(depthIndex, QStringLiteral("41 ft")));
	QCOMPARE(cylinder->depth.mm, feet_to_mm(41.0));

	QSignalSpy dataChangedSpy(model, &CylindersModel::dataChanged);
	QVERIFY(!model->setData(depthIndex, QStringLiteral("invalid")));
	QCOMPARE(cylinder->depth.mm, feet_to_mm(41.0));
	QCOMPARE(dataChangedSpy.count(), 0);
	QVERIFY(!model->setData(depthIndex, QStringLiteral("m")));
	QCOMPARE(cylinder->depth.mm, feet_to_mm(41.0));
	QCOMPARE(dataChangedSpy.count(), 0);
	QVERIFY(!model->setData(depthIndex, QStringLiteral("-3 m")));
	QCOMPARE(cylinder->depth.mm, feet_to_mm(41.0));
	QCOMPARE(dataChangedSpy.count(), 0);

	planner->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
void TestDivePlannerModel::testStoredZeroCylinderDepthDisplay()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;
	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);

	CylindersModel *cylinders = model->cylindersModel();
	QModelIndex depthIndex = cylinders->index(0, CylindersModel::DEPTH);
	plannedDive.cylinders[0].depth = 0_m;

	QCOMPARE(cylinders->data(depthIndex, Qt::DisplayRole).toString(), get_depth_string(0_m, true));

	model->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
void TestDivePlannerModel::testZeroDepthExcludesDecoGas()
{
	DivePlannerPointsModel *planner = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	planner->setPlanMode(DivePlannerPointsModel::PLAN);
	planner->createSimpleDive(&plannedDive);
	CylindersModel *model = planner->cylindersModel();
	model->add();
	QCOMPARE(plannedDive.cylinders.size(), size_t(2));
	cylinder_t *decoCylinder = plannedDive.get_cylinder(1);
	QVERIFY(decoCylinder);
	decoCylinder->cylinder_use = OC_GAS;
	decoCylinder->gasmix.o2 = make_fraction(500);
	decoCylinder->depth = 21_m;
	QModelIndex depthIndex = model->index(1, CylindersModel::DEPTH);

	QVERIFY(model->setData(depthIndex, QStringLiteral("21 m")));
	planner->emitDataChanged();
	const diveplan &manualDepthPlan = planner->getDiveplan();
	QVERIFY(std::any_of(manualDepthPlan.dp.begin(), manualDepthPlan.dp.end(), [](const divedatapoint &point) {
		return point.time == 0 && !point.entered && point.cylinderid == 1 && point.depth.mm == 21000;
	}));

	QVERIFY(model->setData(depthIndex, QStringLiteral("0")));
	planner->emitDataChanged();
	const diveplan &zeroDepthPlan = planner->getDiveplan();
	QVERIFY(std::none_of(zeroDepthPlan.dp.begin(), zeroDepthPlan.dp.end(), [](const divedatapoint &point) {
		return point.time == 0 && !point.entered && point.cylinderid == 1;
	}));

	decoCylinder->cylinder_use = TRAVEL_OC;
	decoCylinder->depth = 21_m;
	planner->emitDataChanged();
	const diveplan &travelGasPlan = planner->getDiveplan();
	QVERIFY(std::none_of(travelGasPlan.dp.begin(), travelGasPlan.dp.end(), [](const divedatapoint &point) {
		return point.time == 0 && !point.entered && point.cylinderid == 1;
	}));

	planner->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
// Recreational plans that require decompression cannot be saved, and become
// saveable again once the entered profile is within the NDL.
void TestDivePlannerModel::testRecreationalPlanSaveAllowed()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = RECREATIONAL;
	prefs.drop_stone_mode = false;
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	diveplan &plan = model->getDiveplan();
	plan.salinity = 10300;
	plan.surface_pressure = 1_atm;
	plan.gfhigh = 100;
	plan.gflow = 100;
	plan.bottomsac = prefs.bottomsac;
	plan.decosac = prefs.decosac;
	model->createSimpleDive(&plannedDive);
	QVERIFY(model->planSaveAllowed());
	QSignalSpy saveAllowedSpy(model, &DivePlannerPointsModel::planSaveAllowedChanged);

	int lastRow = model->rowCount() - 1;
	model->setData(model->index(0, DivePlannerPointsModel::DEPTH), 50);
	model->setData(model->index(lastRow, DivePlannerPointsModel::DEPTH), 50);
	model->setData(model->index(lastRow, DivePlannerPointsModel::RUNTIME), 50);
	QVERIFY(!model->planSaveAllowed());
	QCOMPARE(saveAllowedSpy.last().at(0).toBool(), false);

	model->savePlan();
	QCOMPARE(model->currentMode(), DivePlannerPointsModel::PLAN);

	model->setData(model->index(0, DivePlannerPointsModel::DEPTH), 20);
	model->setData(model->index(lastRow, DivePlannerPointsModel::DEPTH), 20);
	model->setData(model->index(lastRow, DivePlannerPointsModel::RUNTIME), 20);
	QVERIFY(model->planSaveAllowed());
	QCOMPARE(saveAllowedSpy.last().at(0).toBool(), true);

	model->resetPlanState();
	prefs = default_prefs;
}

void TestDivePlannerModel::testPo2LimitsRefreshPlanNotes()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;
	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.display_variations = false;
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);
	// Mirror the desktop planner's cylinder-change connection. Deco pO2
	// already triggers a replan through updateDecoDepths(), but bottom pO2
	// without a best-mix cylinder does not emit a cylinder change.
	QObject desktopConnections;
	connect(model->cylindersModel(), &CylindersModel::dataChanged, &desktopConnections,
		[model] { model->emitDataChanged(); });
	QSignalSpy notes(model, &DivePlannerPointsModel::calculatedPlanNotes);

	// Air at 15 m has a pO2 of about 0.5 bar. Changing the bottom limit
	// must update the notes even if there is no best-mix cylinder.
	PlannerShared::set_bottompo2(0.4);
	QCOMPARE(notes.size(), 1);
	QVERIFY(notes.last().at(0).toString().contains(QStringLiteral("high pO₂ value")));
	PlannerShared::set_bottompo2(1.4);
	QCOMPARE(notes.size(), 2);
	QVERIFY(!notes.last().at(0).toString().contains(QStringLiteral("high pO₂ value")));

	// Make a decompression dive so the calculated (not entered) stops are
	// checked against the deco limit rather than the bottom limit.
	model->setData(model->index(0, DivePlannerPointsModel::DEPTH), 40);
	model->setData(model->index(1, DivePlannerPointsModel::DEPTH), 40);
	model->setData(model->index(1, DivePlannerPointsModel::RUNTIME), 30);
	QVERIFY(std::any_of(model->getDiveplan().dp.begin(), model->getDiveplan().dp.end(), [](const divedatapoint &point) {
		return !point.entered && point.time > 0;
	}));
	// Adding a deco gas lets us verify that automatic switch depths update
	// before the single calculation triggered by the cylinder change.
	model->cylindersModel()->add();
	cylinder_t *decoGas = plannedDive.get_cylinder(1);
	QVERIFY(decoGas);
	decoGas->gasmix.o2 = make_fraction(500);
	decoGas->depth = plannedDive.gas_mod(decoGas->gasmix, { .mbar = prefs.decopo2 }, m_or_ft(3, 10));
	const int oldDepth = decoGas->depth.mm;
	const int beforeDeco = notes.size();
	PlannerShared::set_decopo2(0.2);
	QCOMPARE(notes.size(), beforeDeco + 1);
	QVERIFY(decoGas->depth.mm < oldDepth);
	QVERIFY(notes.last().at(0).toString().contains(QStringLiteral("high pO₂ value")));
	PlannerShared::set_decopo2(1.6);
	QCOMPARE(notes.size(), beforeDeco + 2);
	QCOMPARE(decoGas->depth.mm, oldDepth);
	QVERIFY(!notes.last().at(0).toString().contains(QStringLiteral("high pO₂ value")));

	// A '*' (best-mix) cylinder must still follow the new bottom limit.
	model->cylindersModel()->add();
	cylinder_t *bestMix = plannedDive.get_cylinder(2);
	QVERIFY(bestMix);
	bestMix->bestmix_o2 = true;
	const int beforeBestMix = notes.size();
	PlannerShared::set_bottompo2(1.2);
	QCOMPARE(notes.size(), beforeBestMix + 1);
	int oldO2 = bestMix->gasmix.o2.permille;
	PlannerShared::set_bottompo2(1.4);
	QCOMPARE(notes.size(), beforeBestMix + 2);
	QVERIFY(bestMix->gasmix.o2.permille > oldO2);

	model->resetPlanState();
	prefs = default_prefs;
}

void TestDivePlannerModel::testDesktopPlannerSurfacePressureInitialisation()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	model->setPlanMode(DivePlannerPointsModel::PLAN);

	// The mobile and desktop planners share this model. Simulate a previous
	// mobile altitude plan before starting a desktop plan whose pressure control
	// still displays the sea-level default.
	model->setSurfacePressure({ .mbar = 800 });
	model->createSimpleDive(&plannedDive);
	model->initialiseDesktopPlanSurfacePressure(1_atm);

	QCOMPARE(model->getSurfacePressure().mbar, (1_atm).mbar);
	QCOMPARE(plannedDive.surface_pressure.mbar, (1_atm).mbar);
	QCOMPARE(plannedDive.dcs[0].surface_pressure.mbar, (1_atm).mbar);

	model->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
// Verify that calculatePlan() honours a pre-set non-sea-level surface pressure
// and does not replace it with 1 atm (the old behaviour for a fresh dive object).
// Also verify that the diveplan carries the correct pressure so that a saved
// copy of the dive inherits it (d->surface_pressure = diveplan.surface_pressure).
void TestDivePlannerModel::testMobilePlannerSurfacePressureRetained()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;

	// Simulate the mobile session boundary reset (surface_pressure = 0 means
	// "use sea level") before setting the test pressure.
	model->setSurfacePressure({ .mbar = 0 });

	// Set an 800 mbar surface pressure before calling calculatePlan.
	pressure_t testPressure = { .mbar = 800 };
	model->setSurfacePressure(testPressure);
	QCOMPARE(model->getSurfacePressure().mbar, 800);

	// Minimal cylinder: AL80 filled with air at 200 bar.
	QVariantList cylinders;
	QVariantMap cyl;
	cyl["type"] = QStringLiteral("AL80");
	cyl["mix"] = QStringLiteral("AIR");
	cyl["pressure"] = 200;
	cyl["use"] = 0;
	cylinders.append(cyl);

	// A single 20-minute segment at 14 m.
	QVariantList segments;
	QVariantMap seg;
	seg["depth"] = 14;
	seg["duration"] = 20;
	seg["gas"] = 0;
	seg["setpoint"] = 0;
	seg["divemode"] = 0;
	segments.append(seg);

	QVariantMap result = model->calculatePlan(
		cylinders, segments,
		QStringLiteral("2025-01-01"), QStringLiteral("10:00:00"),
		0 /* OC */, 10300 /* sea water */, false /* don't save */
	);

	QVERIFY(result.value(QStringLiteral("dateTimeValid")).toBool());

	// The surface pressure stored in the model must still be 800 mbar.
	QCOMPARE(model->getSurfacePressure().mbar, 800);

	// getMobilePlannerSurfacePressure() must also return 800 mbar (not the 1013
	// sea-level fallback) because the stored value is non-zero.
	QCOMPARE(model->getMobilePlannerSurfacePressure(), 800);

	// The diveplan itself must carry 800 mbar.  calculatePlan() sets
	// d->surface_pressure = diveplan.surface_pressure before the save step,
	// so confirming diveplan.surface_pressure is sufficient to prove that the
	// saved dive would inherit the correct value.  (Command::addDive is a
	// stub in this test binary, so we cannot observe the saved dive directly.)
	QCOMPARE(model->getDiveplan().surface_pressure.mbar, 800);

	// The plan notes must be non-empty (plan ran successfully).
	QVERIFY(!result.value(QStringLiteral("notes")).toString().isEmpty());

	model->setSurfacePressure({ .mbar = 0 }); // reset for other tests
	prefs = default_prefs;
}

// AI-generated (Claude)
// Exercise the shouldSave=true code path in calculatePlan() to verify that
// the surface pressure is preserved through to diveplan even when the planner
// is asked to persist the result.  Command::addDive is stubbed out in this
// test binary, so we cannot inspect the saved dive record, but we can confirm
// that (a) the call does not crash, (b) the result is valid, and (c)
// diveplan.surface_pressure still carries the pre-set value after the call
// (it is set before the plan() invocation, so a regression in the save path
// would not reset it, but any future bug that cleared it would be caught here).
void TestDivePlannerModel::testMobilePlannerSurfacePressureRetainedWithSave()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;

	model->setSurfacePressure({ .mbar = 0 });

	pressure_t testPressure = { .mbar = 800 };
	model->setSurfacePressure(testPressure);

	QVariantList cylinders;
	QVariantMap cyl;
	cyl["type"] = QStringLiteral("AL80");
	cyl["mix"] = QStringLiteral("AIR");
	cyl["pressure"] = 200;
	cyl["use"] = 0;
	cylinders.append(cyl);

	QVariantList segments;
	QVariantMap seg;
	seg["depth"] = 14;
	seg["duration"] = 20;
	seg["gas"] = 0;
	seg["setpoint"] = 0;
	seg["divemode"] = 0;
	segments.append(seg);

	// shouldSave=true exercises the copy_dive + Command::addDive path.
	// Command::addDive is a stub here; the test verifies the plan completes
	// without crashing and the pressure is preserved in diveplan.
	QVariantMap result = model->calculatePlan(
		cylinders, segments,
		QStringLiteral("2025-01-01"), QStringLiteral("10:00:00"),
		0 /* OC */, 10300 /* sea water */, true /* save */
	);

	QVERIFY(result.value(QStringLiteral("dateTimeValid")).toBool());
	QCOMPARE(model->getDiveplan().surface_pressure.mbar, 800);
	QVERIFY(!result.value(QStringLiteral("notes")).toString().isEmpty());

	model->setSurfacePressure({ .mbar = 0 }); // reset for other tests
	prefs = default_prefs;
}

// AI-generated (Claude)
// Verify that an above-sea-level surface pressure (1050 mbar, within the
// 689–1100 mbar desktop range) is accepted without clamping, that the
// corresponding altitude value the mobile UI would display is 0 (clamped from
// the negative altitude that pressure_to_altitude() returns for > 1013 mbar),
// and that setMobilePlannerAltitudeDisplay/getMobilePlannerAltitudeDisplay round-trip.
void TestDivePlannerModel::testMobilePlannerAboveSeaLevelPressure()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();

	// Set 1050 mbar via the clamped setter.
	model->setMobilePlannerSurfacePressure(1050);
	QCOMPARE(model->getMobilePlannerSurfacePressure(), 1050);

	// pressure_to_altitude(1050 mbar) is negative; getMobilePlannerAltitudeDisplay()
	// must clamp that to 0.
	QCOMPARE(model->getMobilePlannerAltitudeDisplay(), 0);

	// Altitude round-trip: set 1000 m (metric), read back the altitude in display
	// units.  The setter converts m → mm → mbar; the getter converts back.
	// Due to the discrete barometric formula the round-trip may be ±1 display unit.
	prefs.units.length = units::METERS;
	model->setMobilePlannerAltitudeDisplay(1000);
	int rtAlt = model->getMobilePlannerAltitudeDisplay();
	QVERIFY(rtAlt >= 999 && rtAlt <= 1001);

	model->setSurfacePressure({ .mbar = 0 }); // reset for other tests
}

// AI-generated (Claude)
// Helpers shared by the mobile-vs-desktop parity tests below.
namespace {

// A compact, order-preserving fingerprint of a planned profile: the sequence
// of (time, depth) sample points plus the total runtime.  Two plans that
// produce the same descent, bottom time and decompression schedule have the
// same fingerprint.
struct PlanFingerprint {
	int durationSeconds = 0;
	std::vector<std::pair<int, int>> samples; // (time.seconds, depth.mm)

	bool operator==(const PlanFingerprint &o) const
	{
		return durationSeconds == o.durationSeconds && samples == o.samples;
	}
};

PlanFingerprint fingerprintDive(const dive &d)
{
	PlanFingerprint fp;
	if (!d.dcs.empty()) {
		for (const struct sample &s : d.dcs[0].samples)
			fp.samples.emplace_back(s.time.seconds, s.depth.mm);
	}
	// Use the last sample time as the runtime so both the desktop and mobile
	// fingerprints define duration identically (the mobile path derives it from
	// the profile sample list it returns to the UI).
	fp.durationSeconds = fp.samples.empty() ? 0 : fp.samples.back().first;
	return fp;
}

// Run the mobile planner path (calculatePlan) with a single bottom segment and
// return the fingerprint of the resulting planned dive, built from the profile
// samples the model returns to the QML UI.
PlanFingerprint runMobilePlan(DivePlannerPointsModel *model, int surfacePressureMbar,
			      int bottomDepthM, int bottomRuntimeMin, int salinity)
{
	model->setSurfacePressure({ .mbar = surfacePressureMbar });

	QVariantList cylinders;
	QVariantMap cyl;
	cyl["type"] = QStringLiteral("D12 232 bar");
	cyl["mix"] = QStringLiteral("AIR");
	cyl["pressure"] = 232;
	cyl["use"] = 0;
	cylinders.append(cyl);

	QVariantList segments;
	QVariantMap seg;
	seg["depth"] = bottomDepthM;
	seg["duration"] = bottomRuntimeMin;
	seg["gas"] = 0;
	seg["setpoint"] = 0;
	seg["divemode"] = 0;
	segments.append(seg);

	QVariantMap result = model->calculatePlan(cylinders, segments,
		QStringLiteral("2025-01-01"), QStringLiteral("10:00:00"),
		0 /* OC */, salinity, false /* don't save */);

	PlanFingerprint fp;
	const QVariantList profile = result.value(QStringLiteral("profile")).toList();
	int lastTime = 0;
	for (const QVariant &pv : profile) {
		QVariantMap pt = pv.toMap();
		int t = pt.value(QStringLiteral("time")).toInt();
		fp.samples.emplace_back(t, pt.value(QStringLiteral("depth")).toInt());
		lastTime = t;
	}
	fp.durationSeconds = lastTime;
	return fp;
}

// Run the desktop planner path (createSimpleDive + model edits) with the same
// single bottom segment and return the fingerprint of the resulting dive.
// This mirrors how the desktop planner UI drives the model.
PlanFingerprint runDesktopPlan(DivePlannerPointsModel *model, dive &plannedDive,
			       int surfacePressureMbar, int bottomDepthM, int bottomRuntimeMin,
			       int salinity)
{
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	diveplan &plan = model->getDiveplan();
	plan.salinity = salinity;
	plan.gflow = prefs.gflow;
	plan.gfhigh = prefs.gfhigh;
	plan.bottomsac = prefs.bottomsac;
	plan.decosac = prefs.decosac;
	// The desktop planner seeds the conservatism spinbox from the preference
	// at setup; replicate that so the two paths start from the same value.
	model->setVpmbConservatism(prefs.vpmb_conservatism);

	model->createSimpleDive(&plannedDive);
	// The desktop planner starts at the pressure already displayed by its
	// atmospheric-pressure widget, then the user changes it to the selected
	// altitude. This must update both the diveplan and temporary dive.
	model->initialiseDesktopPlanSurfacePressure(1_atm);
	model->setSurfacePressure({ .mbar = surfacePressureMbar });

	// Replace the default rows with a single bottom segment matching the mobile
	// input: depth and cumulative runtime.
	std::vector<int> allRows;
	for (int i = 0; i < model->rowCount(); i++)
		allRows.push_back(i);
	model->removeSelectedPoints(allRows);
	model->addStop(m_or_ft(bottomDepthM, bottomDepthM * 3), bottomRuntimeMin * 60);

	return fingerprintDive(plannedDive);
}

} // namespace

// AI-generated (Claude)
// Regression test for issue #4941: at altitude the mobile planner (calculatePlan)
// must produce the same descent, bottom time and decompression schedule as the
// desktop planner.  The user-visible discrepancy was driven by the mobile
// descent being rounded to whole minutes (see testMobilePlannerDescentMatchesDesktop);
// the effect was more pronounced at altitude, which is why #4941 surfaced it.
// This end-to-end altitude case guards against any future reintroduction.
void TestDivePlannerModel::testMobilePlannerAltitudeMatchesDesktop()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = true;
	prefs.last_stop = false; // 3 m last stop
	prefs.descrate = lrint(10 * 1000.0 / 60.0); // 10 m/min, in mm/s
	prefs.bottomsac = 20000;
	prefs.decosac = 20000;
	prefs.decopo2 = 1600;
	prefs.gflow = 40;
	prefs.gfhigh = 70;
	prefs.display_variations = false;

	const int altitudePressure = 891; // 891 mbar ~ 1000 m

	PlanFingerprint desktop = runDesktopPlan(model, plannedDive, altitudePressure, 50, 30, 10000);
	model->resetPlanState();

	PlanFingerprint mobile = runMobilePlan(model, altitudePressure, 50, 30, 10000);
	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;

	QCOMPARE(mobile.durationSeconds, desktop.durationSeconds);
	QVERIFY(mobile == desktop);
}

// AI-generated (Claude)
// Regression test for the descent-rounding bug (reported by @glance- on #4941):
// the mobile planner used to round the "drop to first depth" descent up to whole
// minutes in display units and subtract that from the bottom time, diverging
// from the desktop planner's mm/s-resolution descent.  Use a descent rate that
// does not divide evenly into minutes (18 m/min over 50 m = 2.78 min) so the
// old rounding would differ.
void TestDivePlannerModel::testMobilePlannerDescentMatchesDesktop()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = true;
	prefs.last_stop = false;
	prefs.descrate = lrint(18 * 1000.0 / 60.0); // 18 m/min -> 50 m takes 2.78 min
	prefs.bottomsac = 20000;
	prefs.decosac = 20000;
	prefs.decopo2 = 1600;
	prefs.gflow = 40;
	prefs.gfhigh = 70;
	prefs.display_variations = false;

	PlanFingerprint desktop = runDesktopPlan(model, plannedDive, 1013, 50, 25, 10300);
	model->resetPlanState();

	PlanFingerprint mobile = runMobilePlan(model, 1013, 50, 25, 10300);
	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;

	QCOMPARE(mobile.durationSeconds, desktop.durationSeconds);
	QVERIFY(mobile == desktop);
}

// AI-generated (Claude)
// Investigation outcome for the VPM-B conservatism question (ticket #182): the
// mobile planner reads conservatism from the preference, and the desktop planner
// seeds its conservatism spinbox from the same preference at setup, so the two
// already agree for identical configuration.  This test pins that behaviour: the
// diveplan the mobile path builds must carry prefs.vpmb_conservatism, so a future
// refactor cannot silently use a different source.
void TestDivePlannerModel::testMobilePlannerUsesPreferenceVpmbConservatism()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = VPMB;
	prefs.vpmb_conservatism = 2; // deliberately not the default (3)
	prefs.drop_stone_mode = false;
	prefs.display_variations = false;

	model->setSurfacePressure({ .mbar = 0 });

	QVariantList cylinders;
	QVariantMap cyl;
	cyl["type"] = QStringLiteral("AL80");
	cyl["mix"] = QStringLiteral("AIR");
	cyl["pressure"] = 200;
	cyl["use"] = 0;
	cylinders.append(cyl);

	QVariantList segments;
	QVariantMap seg;
	seg["depth"] = 30;
	seg["duration"] = 25;
	seg["gas"] = 0;
	seg["setpoint"] = 0;
	seg["divemode"] = 0;
	segments.append(seg);

	model->calculatePlan(cylinders, segments,
		QStringLiteral("2025-01-01"), QStringLiteral("10:00:00"),
		0, 10300, false);

	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;

	QCOMPARE(model->getDiveplan().vpmb_conservatism, 2);
}

// AI-generated (Claude)
// Parity test for deco-gas switch depths at altitude.  The mobile planner used
// to compute its own gas-switch "time == 0" waypoints with gas_mod() rounded to
// 1 m, before the dive computer's surface pressure was set, so at altitude they
// were computed at sea level and with the wrong rounding.  The desktop planner
// instead uses each cylinder's switch depth, which reset_cylinders() derives
// from the shared calculate_deco_switch_depth() (gas_mod rounded to 3 m / 10 ft)
// at the plan's surface pressure.  Verify the mobile waypoints now equal the
// shared helper evaluated at the altitude pressure for every OC gas, so the two
// planners select the same gas-switch depths.
void TestDivePlannerModel::testMobilePlannerDecoGasSwitchDepthMatchesDesktop()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;
	prefs.decopo2 = 1600;
	prefs.display_variations = false;

	const int altitude = 700; // mbar, well above sea level

	model->setSurfacePressure({ .mbar = altitude });

	QVariantList cylinders;
	QVariantMap bottom, deco;
	bottom["type"] = QStringLiteral("D12 232 bar");
	bottom["mix"] = QStringLiteral("AIR");
	bottom["pressure"] = 232;
	bottom["use"] = 0;
	deco["type"] = QStringLiteral("AL80");
	deco["mix"] = QStringLiteral("EAN50");
	deco["pressure"] = 200;
	deco["use"] = 0;
	cylinders.append(bottom);
	cylinders.append(deco);

	QVariantList segments;
	QVariantMap seg;
	seg["depth"] = 40;
	seg["duration"] = 25;
	seg["gas"] = 0;
	seg["setpoint"] = 0;
	seg["divemode"] = 0;
	segments.append(seg);

	model->calculatePlan(cylinders, segments,
		QStringLiteral("2025-01-01"), QStringLiteral("10:00:00"),
		0, 10300, false);

	// Expected switch depths from the shared helper, at the altitude pressure.
	dive ref;
	ref.surface_pressure = { .mbar = altitude };
	ref.dcs[0].surface_pressure = { .mbar = altitude };
	gasmix air = { .o2 = { .permille = 0 }, .he = { .permille = 0 } };
	gasmix ean50 = { .o2 = { .permille = 500 }, .he = { .permille = 0 } };
	depth_t airDepth = calculate_deco_switch_depth(&ref, air);
	depth_t ean50Depth = calculate_deco_switch_depth(&ref, ean50);

	int airWaypoint = -1, ean50Waypoint = -1;
	for (const auto &p : model->getDiveplan().dp) {
		if (p.time == 0 && !p.entered) {
			if (p.cylinderid == 0)
				airWaypoint = p.depth.mm;
			else if (p.cylinderid == 1)
				ean50Waypoint = p.depth.mm;
		}
	}
	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;

	QCOMPARE(airWaypoint, airDepth.mm);
	QCOMPARE(ean50Waypoint, ean50Depth.mm);

	// The EAN50 switch depth must be altitude-correct (deeper than sea level),
	// so this fails if the waypoint were computed at 1 atm.
	dive sea;
	sea.surface_pressure = 1_atm;
	sea.dcs[0].surface_pressure = 1_atm;
	QVERIFY(ean50Depth.mm > calculate_deco_switch_depth(&sea, ean50).mm);
}

// AI-generated (Claude)
// Regression test for the desktop planner cylinder-depth staleness bug: after
// createSimpleDive() sets cylinder switch depths at sea level and the user then
// changes the surface pressure to altitude, setSurfacePressure() must recompute
// auto-derived cylinder depths so that createTemporaryPlan() inserts correct
// altitude time-zero gas waypoints.  This matters for multi-cylinder desktop
// plans where the deco gas switch depth affects the decompression algorithm.
void TestDivePlannerModel::testDesktopPlannerCylinderDepthsUpdatedOnPressureChange()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;
	prefs.decopo2 = 1600;
	prefs.display_variations = false;

	const int altitudePressure = 700; // mbar

	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);
	// createSimpleDive() uses a single air cylinder; add an EAN50 deco cylinder
	// to give a meaningful time-zero waypoint whose depth depends on surface pressure.
	// Give it a realistic auto-derived depth (what the real "add cylinder" path
	// in fill_default_cylinder()/create_new_cylinder() would compute at sea
	// level) rather than a bare default-constructed zero: a pre-existing zero
	// is treated by updateCylinderDepthsForPressure() as an explicit "switch
	// disabled" override and is intentionally never recomputed, so leaving it
	// at zero here would not exercise the recompute path this test targets.
	gasmix ean50 = { .o2 = { .permille = 500 }, .he = { .permille = 0 } };
	cylinder_t decoCyl;
	decoCyl.gasmix = ean50;
	decoCyl.cylinder_use = OC_GAS;
	decoCyl.depth = calculate_deco_switch_depth(&plannedDive, ean50); // sea level, surface_pressure still unset
	plannedDive.cylinders.add(1, decoCyl); // insert at index 1

	// Mimic the desktop UI setup: seed from the spinbox value, then apply altitude.
	model->initialiseDesktopPlanSurfacePressure(1_atm);
	model->setSurfacePressure({ .mbar = altitudePressure });

	// Build the expected altitude depth via the shared helper.
	dive ref;
	ref.surface_pressure = { .mbar = altitudePressure };
	ref.dcs[0].surface_pressure = { .mbar = altitudePressure };
	depth_t expectedDepth = calculate_deco_switch_depth(&ref, ean50);

	// Sea-level depth for comparison — the bug produced this instead.
	dive seaRef;
	seaRef.surface_pressure = 1_atm;
	seaRef.dcs[0].surface_pressure = 1_atm;
	depth_t seaLevelDepth = calculate_deco_switch_depth(&seaRef, ean50);

	// Capture the depth before cleanup; the cylinder was just added so null is impossible.
	cylinder_t *ean50Cyl = plannedDive.get_cylinder(1);
	int actualDepth = ean50Cyl ? ean50Cyl->depth.mm : -1;

	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;

	// The EAN50 cylinder depth must now reflect altitude, not sea level.
	QVERIFY(ean50Cyl != nullptr);
	QCOMPARE(actualDepth, expectedDepth.mm);
	// Altitude depth must differ from sea-level depth (confirms the test is meaningful).
	QVERIFY(expectedDepth.mm != seaLevelDepth.mm);
}

// AI-generated (Claude)
// Regression test: an explicit "0" switch depth (user disables a deco-gas
// switch, see testZeroDepthExcludesDecoGas) must survive a surface-pressure
// change.  updateCylinderDepthsForPressure() must not mistake the explicit
// override for an auto-derived depth that happened to match the old MOD and
// recompute it, which would silently re-enable the switch (e.g. via the
// mobile altitude control) at the new pressure.
//
// This checks the generated diveplan rather than the live cylinder_t::depth
// field: a separate, pre-existing issue in reset_cylinders() (invoked from
// plan()'s create_dive_from_plan() on every plan recomputation, not part of
// this fix) unconditionally re-derives the MOD for any cylinder whose depth
// is currently 0, so the persisted cylinder depth cannot be asserted to
// remain 0 across a further plan recomputation. That is independent of, and
// not fixed by, this change. The diveplan content is what the fixed function
// in this file is actually responsible for, and is unaffected by that
// separate issue because createTemporaryPlan() runs (and captures the
// still-zero depth) before plan() calls reset_cylinders().
void TestDivePlannerModel::testExplicitZeroCylinderDepthSurvivesPressureChange()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;
	prefs.decopo2 = 1600;
	prefs.display_variations = false;

	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);
	// Seed the desktop pressure control, as diveplanner.cpp does at plan setup,
	// before the user touches any cylinder.
	model->initialiseDesktopPlanSurfacePressure(1_atm);

	// Add an EAN50 deco cylinder and explicitly disable its gas switch by
	// setting its depth to "0" through the cylinder editor, exactly as
	// testZeroDepthExcludesDecoGas does.
	CylindersModel *cylinderModel = model->cylindersModel();
	cylinderModel->add();
	QCOMPARE(plannedDive.cylinders.size(), size_t(2));
	cylinder_t *decoCylinder = plannedDive.get_cylinder(1);
	QVERIFY(decoCylinder);
	decoCylinder->cylinder_use = OC_GAS;
	decoCylinder->gasmix.o2 = make_fraction(500);
	decoCylinder->depth = 21_m;
	QModelIndex depthIndex = cylinderModel->index(1, CylindersModel::DEPTH);
	QVERIFY(cylinderModel->setData(depthIndex, QStringLiteral("0")));
	QCOMPARE(decoCylinder->depth.mm, 0);

	// Now change the surface pressure, as the mobile altitude control would.
	model->setSurfacePressure({ .mbar = 700 });

	// The deco-gas switch must remain disabled: no time-zero waypoint for the
	// explicitly-zeroed cylinder in the plan generated at the new pressure.
	const diveplan &plan = model->getDiveplan();
	QVERIFY(std::none_of(plan.dp.begin(), plan.dp.end(), [](const divedatapoint &point) {
		return point.time == 0 && !point.entered && point.cylinderid == 1;
	}));

	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;
}

// AI-generated (Claude)
// Regression test: updateCylinderDepthsForPressure() mutates cyl.depth directly
// on the dive's cylinder objects, but that bypasses CylindersModel's own change
// notification. setSurfacePressure() and initialiseDesktopPlanSurfacePressure()
// must emit CylindersModel::dataChanged (not just DivePlannerPointsModel's own
// dataChanged) after recomputing depths, otherwise the desktop cylinder table
// view keeps showing a stale switch depth/MOD after an altitude change even
// though the plan itself now uses the recomputed value.
void TestDivePlannerModel::testCylindersModelRefreshedOnPressureChange()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	prefs = default_prefs;
	prefs.unit_system = METRIC;
	prefs.units = SI_units;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.drop_stone_mode = false;
	prefs.decopo2 = 1600;
	prefs.display_variations = false;

	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);
	// Add an auto-derived EAN50 deco cylinder, as in
	// testDesktopPlannerCylinderDepthsUpdatedOnPressureChange, so that
	// changing the surface pressure actually recomputes a cylinder depth.
	// A bare default-constructed zero depth would instead be treated as an
	// explicit "switch disabled" override and never recomputed by
	// updateCylinderDepthsForPressure(), so seed a realistic sea-level
	// auto-derived depth first.
	gasmix ean50 = { .o2 = { .permille = 500 }, .he = { .permille = 0 } };
	cylinder_t decoCyl;
	decoCyl.gasmix = ean50;
	decoCyl.cylinder_use = OC_GAS;
	decoCyl.depth = calculate_deco_switch_depth(&plannedDive, ean50); // sea level, surface_pressure still unset
	plannedDive.cylinders.add(1, decoCyl);

	model->initialiseDesktopPlanSurfacePressure(1_atm);

	CylindersModel *cylinderModel = model->cylindersModel();
	QVERIFY(cylinderModel);
	QSignalSpy dataChangedSpy(cylinderModel, &CylindersModel::dataChanged);

	// Mimic an altitude change made through the desktop pressure control.
	model->setSurfacePressure({ .mbar = 700 });

	QVERIFY2(dataChangedSpy.count() > 0,
		 "CylindersModel::dataChanged was not emitted after a surface "
		 "pressure change recomputed cylinder switch depths");

	model->setSurfacePressure({ .mbar = 0 });
	model->resetPlanState();
	prefs = default_prefs;
}

// Stubs for symbols referenced by libraries linked into TestDivePlannerModel
// but not available without the full desktop-widgets and commands libraries.

// Command stubs — these are referenced by various qt-models source files
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
void renameCylinderType(const std::string &, const std::string &) {}

} // namespace Command

// MapWidget stubs — referenced by maplocationmodel.cpp and core/divefilter.cpp
#ifdef MAP_SUPPORT
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

// MainWindow stub — referenced by core/divefilter.cpp
MainWindow *MainWindow::instance()
{
	return nullptr;
}
#endif

QTEST_GUILESS_MAIN(TestDivePlannerModel)
