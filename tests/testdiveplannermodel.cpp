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
	QCOMPARE(gas.toString(), QStringLiteral("AIR"));

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
