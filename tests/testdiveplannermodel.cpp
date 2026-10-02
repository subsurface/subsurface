// SPDX-License-Identifier: GPL-2.0
#include "testdiveplannermodel.h"
#include "qt-models/diveplannermodel.h"
#include "core/subsurfacestartup.h"
#include "commands/command.h"
#include "core/divelog.h"
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

// Stubs for symbols referenced by libraries linked into TestDivePlannerModel
// but not available without the full desktop-widgets and commands libraries.

// Command stubs — these are referenced by various qt-models source files
namespace Command {

void addDive(std::unique_ptr<dive>, bool, bool) {}
void importDives(struct divelog *, int, const QString &) {}
void replanDive(dive *) {}
int editCylinder(int, cylinder_t, EditCylinderType, bool) { return 0; }
void editSensors(int, int, int) {}
void editDiveSiteName(dive_site *, const QString &) {}
void editDiveSiteDescription(dive_site *, const QString &) {}
void removePictures(const std::vector<PictureListForDeletion> &) {}
int editWeight(int, weightsystem_t, bool) { return 0; }

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
