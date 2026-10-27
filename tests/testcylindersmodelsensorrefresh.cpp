// SPDX-License-Identifier: GPL-2.0
//
// Regression tests for the Equipment tab's cylinder table not refreshing
// after a tank-sensor reassignment made from somewhere other than that
// table's own "Tank sensor" column editor (e.g. the profile's "Assign tank
// sensor to cylinder" context-menu entry).
//
// Root cause: CylindersModel was only connected to DiveListNotifier's
// cylindersReset/cylinderAdded/cylinderRemoved/cylinderEdited signals.
// Sensor-mapping edits (Command::editSensors(), via the EditSensors undo
// command) emit diveComputerEdited instead, which CylindersModel never
// listened to. Editing the "Tank sensor" column directly in the Equipment
// tab appeared to work only because QAbstractItemView repaints the cell
// under the editor as a side effect of closing that editor, independent of
// any model signal; any other trigger of the same command left the table
// stale.
//
// These tests exercise the fix directly at the signal level, since the
// Command/undo layer is not linked into the test binary (see testbase.cpp).

#include "testcylindersmodelsensorrefresh.h"

#include "commands/command.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divesite.h"
#include "core/equipment.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/tanksensormapping.h"
#include "qt-models/cylindermodel.h"
#if defined(MAP_SUPPORT)
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

#include <QSignalSpy>

// A sensor reassignment on the model's own dive/divecomputer must make the
// model emit dataChanged for a range that includes the SENSORS column, no
// matter what triggered the DiveListNotifier::diveComputerEdited signal.
void TestCylindersModelSensorRefresh::testDiveComputerEditedRefreshesOwnDive()
{
	dive d;
	// Mark both cylinders manually added so they are visible rows in CylindersModel
	// (unused, pressure-less cylinders are hidden - see first_hidden_cylinder()).
	// Otherwise the model would have zero rows and the assertions below could pass
	// vacuously, without actually exercising any SENSORS cell.
	d.get_or_create_cylinder(0)->manually_added = true;
	d.get_or_create_cylinder(1)->manually_added = true;
	d.dcs[0].tank_sensor_mappings = { tank_sensor_mapping{ 0, 1 } };

	CylindersModel model(false);
	model.updateDive(&d, 0);

	QCOMPARE(model.rowCount(), 2);
	QCOMPARE(model.data(model.index(0, CylindersModel::SENSORS)).toString(), QString("<none>"));
	QCOMPARE(model.data(model.index(1, CylindersModel::SENSORS)).toString(), QString("1"));

	QSignalSpy spy(&model, &CylindersModel::dataChanged);

	// Simulate what Command::editSensors()'s EditSensors::redo()/undo() does:
	// change the mapping, then notify via diveComputerEdited. This is exactly
	// the signal the profile's context-menu action triggers, and that the
	// Equipment tab's own in-place "Tank sensor" column edit also triggers
	// (that path additionally benefits from the editor-close repaint, but
	// must not rely on it alone).
	d.dcs[0].tank_sensor_mappings[0].cylinder_index = 0;
	emit diveListNotifier.diveComputerEdited(d, d.dcs[0]);

	QCOMPARE(spy.count(), 1);
	QList<QVariant> args = spy.takeFirst();
	QModelIndex topLeft = args.at(0).toModelIndex();
	QModelIndex bottomRight = args.at(1).toModelIndex();
	QVERIFY(topLeft.column() <= CylindersModel::SENSORS);
	QVERIFY(bottomRight.column() >= CylindersModel::SENSORS);
	QVERIFY(bottomRight.row() >= 1);

	// The sensor must now show as moved to cylinder 0, and cylinder 1 as unmapped:
	// this is the actual data the dataChanged signal above is supposed to cover.
	QCOMPARE(model.data(model.index(0, CylindersModel::SENSORS)).toString(), QString("1"));
	QCOMPARE(model.data(model.index(1, CylindersModel::SENSORS)).toString(), QString("<none>"));
}

// A diveComputerEdited signal for an unrelated dive (or an unrelated
// divecomputer on the model's own dive) must not cause a refresh.
void TestCylindersModelSensorRefresh::testDiveComputerEditedIgnoresOtherDive()
{
	dive d;
	d.get_or_create_cylinder(0);

	dive other;
	other.get_or_create_cylinder(0);

	CylindersModel model(false);
	model.updateDive(&d, 0);

	QSignalSpy spy(&model, &CylindersModel::dataChanged);

	emit diveListNotifier.diveComputerEdited(other, other.dcs[0]);

	QCOMPARE(spy.count(), 0);
}

// Stubs for symbols referenced (but never actually exercised) by other
// desktop qt-models source files that end up linked into this test binary
// alongside CylindersModel, because they all share subsurface_models_desktop.
// See testdiveplannermodel.cpp / testtankinfomodel.cpp for the same pattern.
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
void renameCylinderType(const std::string &, const std::string &) {}

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

QTEST_GUILESS_MAIN(TestCylindersModelSensorRefresh)
