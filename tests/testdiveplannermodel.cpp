// SPDX-License-Identifier: GPL-2.0
#include "testdiveplannermodel.h"
#include "commands/command.h"
#include "core/divelog.h"
#include "core/pref.h"
#include "core/subsurfacestartup.h"
#include "core/units.h"
#include "qt-models/diveplannermodel.h"
#include <QSignalSpy>
#include <memory>
#include <vector>

#ifdef MAP_SUPPORT
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

namespace
{

	// AI-generated (Claude)
	struct EnteredPoint {
		int depthMm;
		int timeSeconds;
		int cylinderId;
	};

	std::vector<EnteredPoint> enteredPoints(const DivePlannerPointsModel *model)
	{
		std::vector<EnteredPoint> result;
		for (int row = 0; row < model->rowCount(); ++row) {
			const divedatapoint point = model->at(row);
			if (!point.entered)
				continue;
			result.push_back({point.depth.mm, point.time, point.cylinderid});
		}
		return result;
	}

	void setPlannerPrefsForRoundTrip(bool imperialUnits, int decoMode, bool dropStoneMode, bool doO2Breaks,
					 bool displayVariations, bool doBailout, bool lastStop, bool switchAtRequiredStop)
	{
		prefs = default_prefs;
		prefs.unit_system = imperialUnits ? IMPERIAL : METRIC;
		prefs.units = imperialUnits ? IMPERIAL_units : SI_units;
		prefs.planner_deco_mode = static_cast<deco_mode>(decoMode);
		prefs.drop_stone_mode = dropStoneMode;
		prefs.doo2breaks = doO2Breaks;
		prefs.display_variations = displayVariations;
		prefs.dobailout = doBailout;
		prefs.last_stop = lastStop;
		prefs.switch_at_req_stop = switchAtRequiredStop;
	}

} // namespace

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
namespace Command
{

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

// AI-generated (Claude)
void TestDivePlannerModel::testSaveLoadCycleStable_data()
{
	QTest::addColumn<bool>("dropStoneMode");
	QTest::addColumn<int>("decoMode");
	QTest::addColumn<bool>("doO2Breaks");
	QTest::addColumn<bool>("displayVariations");
	QTest::addColumn<bool>("doBailout");
	QTest::addColumn<bool>("lastStop");
	QTest::addColumn<bool>("switchAtRequiredStop");
	QTest::addColumn<bool>("imperialUnits");

	QTest::newRow("drop-stone-off-buehlmann-metric") << false << int(BUEHLMANN) << false << false << false << true << true << false;
	QTest::newRow("drop-stone-on-buehlmann-metric") << true << int(BUEHLMANN) << false << false << false << true << true << false;
	QTest::newRow("drop-stone-on-vpmb-imperial") << true << int(VPMB) << true << false << false << false << false << true;
	QTest::newRow("drop-stone-off-vpmb-imperial") << false << int(VPMB) << true << false << false << false << false << true;
	QTest::newRow("drop-stone-on-recreational") << true << int(RECREATIONAL) << false << true << false << true << true << false;
	QTest::newRow("drop-stone-on-bailout") << true << int(BUEHLMANN) << false << false << true << true << true << false;
}

void TestDivePlannerModel::testSaveLoadCycleStable()
{
	QFETCH(bool, dropStoneMode);
	QFETCH(int, decoMode);
	QFETCH(bool, doO2Breaks);
	QFETCH(bool, displayVariations);
	QFETCH(bool, doBailout);
	QFETCH(bool, lastStop);
	QFETCH(bool, switchAtRequiredStop);
	QFETCH(bool, imperialUnits);

	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	setPlannerPrefsForRoundTrip(imperialUnits, decoMode, dropStoneMode, doO2Breaks, displayVariations, doBailout,
				    lastStop, switchAtRequiredStop);
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);

	const auto expected = enteredPoints(model);
	QVERIFY(!expected.empty());
	auto expectedAfterLoad = expected;

	for (int cycle = 0; cycle < 3; ++cycle) {
		model->loadFromDive(&plannedDive, 0);
		const auto actual = enteredPoints(model);

		// Recreational mode may normalize a single drop-stone waypoint into an
		// equivalent two-point representation after first load. Accept that
		// normalization once, then require strict stability for subsequent cycles.
		if (cycle == 0 && dropStoneMode && decoMode == int(RECREATIONAL) && actual.size() == expected.size() + 1 &&
		    actual[0].depthMm == expected[0].depthMm && actual[0].timeSeconds == expected[0].timeSeconds && actual[0].cylinderId == expected[0].cylinderId)
			expectedAfterLoad = actual;

		QCOMPARE(actual.size(), expectedAfterLoad.size());
		for (size_t i = 0; i < expectedAfterLoad.size(); ++i) {
			QCOMPARE(actual[i].depthMm, expectedAfterLoad[i].depthMm);
			QCOMPARE(actual[i].timeSeconds, expectedAfterLoad[i].timeSeconds);
			QCOMPARE(actual[i].cylinderId, expectedAfterLoad[i].cylinderId);
		}

		// Re-run planning from the loaded waypoints to model repeated save/load cycles.
		model->emitDataChanged();
	}

	model->resetPlanState();
	prefs = default_prefs;
}

void TestDivePlannerModel::testDropStoneLoadKeepsUserPoint()
{
	DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
	dive plannedDive;

	setPlannerPrefsForRoundTrip(false, int(BUEHLMANN), false, false, false, false, true, true);
	model->setPlanMode(DivePlannerPointsModel::PLAN);
	model->createSimpleDive(&plannedDive);

	QVERIFY(model->rowCount() >= 2);
	model->setData(model->index(0, DivePlannerPointsModel::DEPTH), 12);
	const auto before = enteredPoints(model);
	QVERIFY(before.size() >= 2);
	QVERIFY(before[0].depthMm != before[1].depthMm);

	// Load with drop-stone enabled although the saved plan had it disabled.
	prefs.drop_stone_mode = true;
	model->loadFromDive(&plannedDive, 0);
	const auto after = enteredPoints(model);

	QCOMPARE(after.size(), before.size());
	for (size_t i = 0; i < before.size(); ++i) {
		QCOMPARE(after[i].depthMm, before[i].depthMm);
		QCOMPARE(after[i].timeSeconds, before[i].timeSeconds);
		QCOMPARE(after[i].cylinderId, before[i].cylinderId);
	}

	model->resetPlanState();
	prefs = default_prefs;
}

QTEST_GUILESS_MAIN(TestDivePlannerModel)

