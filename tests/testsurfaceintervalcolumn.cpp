// SPDX-License-Identifier: GPL-2.0
//
// Tests for the optional "Surface interval" column of the dive list.
//
// The test log is built in code, so that it can contain dives that start
// at identical times, which the XML test files do not. All times are in
// seconds. Start time / duration / interval before the dive:
//
//   A  10000 / 1800 / none (first dive)
//   B  17200 / 1800 / 5400 (1h 30min after the end of A)
//   C  17200 / 600  / 5400 (identical start time as B; B is sorted before C by number,
//                           but the interval is still calculated against A, because
//                           B does not start *before* C)
//   D  17500 / 600  / 0    (starts during C)
//   E  21685 / 1200 / 3585 (59min 45s after the end of D, shows as "1h 0min")
//   F  121685 / 600 / 98800 (1d 3h 26min 40s after the end of E, shows as "1d 3h 27min")
//   G  F end + 6 days / 600 / 518400 ("more than 4 days")
//   H  G end + 4e9 / 600 / 4e9 (does not fit in 32 bits)

#include "testsurfaceintervalcolumn.h"
#include "core/dive.h"
#include "core/divefilter.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/pref.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/trip.h"
#include "qt-models/divetripmodel.h"

#include "commands/command.h"
#if defined(MAP_SUPPORT)
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

#include <QSignalSpy>

static const int SECONDS_PER_DAY = 24 * 60 * 60;
static const timestamp_t START_A = 10000;
static const timestamp_t START_B = START_A + 1800 + 5400;
static const timestamp_t START_D = START_B + 300;
static const timestamp_t START_E = START_D + 600 + 3585;
static const timestamp_t START_F = START_E + 1200 + 98800;
static const timestamp_t START_G = START_F + 600 + 6 * SECONDS_PER_DAY;
static const timestamp_t START_H = START_G + 600 + 4000000000LL;

static dive *addDive(int number, timestamp_t when, int duration)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;
	d->duration.seconds = duration;
	return divelog.dives.register_dive(std::move(d));
}

static dive *diveByNumber(int number)
{
	for (auto &d: divelog.dives) {
		if (d->number == number)
			return d.get();
	}
	return nullptr;
}

// Row of the dive in a list model, or -1 if it is not shown.
// In a tree model, only top-level dives are found.
static int rowOf(const DiveTripModelBase &model, const dive *d)
{
	for (int row = 0; row < model.rowCount(); ++row) {
		if (model.index(row, 0).data(DiveTripModelBase::DIVE_ROLE).value<dive *>() == d)
			return row;
	}
	return -1;
}

static QString surfaceInterval(const DiveTripModelBase &model, int row)
{
	return model.index(row, DiveTripModelBase::SURFACE_INTERVAL).data(Qt::DisplayRole).toString();
}

void TestSurfaceIntervalColumn::init()
{
	prefs = default_prefs;
	divelog.clear();

	addDive(1, START_A, 1800);
	addDive(2, START_B, 1800);
	addDive(3, START_B, 600);
	addDive(4, START_D, 600);
	addDive(5, START_E, 1200);
	addDive(6, START_F, 600);
	addDive(7, START_G, 600);
	addDive(8, START_H, 600);
	DiveFilter::instance()->reset();
}

void TestSurfaceIntervalColumn::cleanup()
{
	divelog.clear();
	DiveFilter::instance()->reset();
}

// The binary-search based lookup must give exactly what the linear
// search of get_surface_interval() gives, for every dive of the log.
void TestSurfaceIntervalColumn::testLookupMatchesLinearSearch()
{
	QCOMPARE(divelog.dives.size(), 8);
	for (auto &d: divelog.dives)
		QCOMPARE(divelog.dives.get_surface_interval_before(*d), divelog.dives.get_surface_interval(d->when));
}

// Many dives with identical start times, as can happen with imported dives without a
// valid time stamp, are all handled like the first dive of such a block.
void TestSurfaceIntervalColumn::testLookupWithManyIdenticalStartTimes()
{
	for (int i = 0; i < 50; ++i)
		addDive(100 + i, START_E, 100 + i);
	for (auto &d: divelog.dives)
		QCOMPARE(divelog.dives.get_surface_interval_before(*d), divelog.dives.get_surface_interval(d->when));
	// All of the 51 dives starting at START_E have dive 4 as predecessor
	QCOMPARE(divelog.dives.get_surface_interval_before(*diveByNumber(149)), timestamp_t(3585));
	QCOMPARE(divelog.dives.get_surface_interval_before(*diveByNumber(5)), timestamp_t(3585));
}

void TestSurfaceIntervalColumn::testLookupValues()
{
	auto interval = [](int number) { return divelog.dives.get_surface_interval_before(*diveByNumber(number)); };
	QCOMPARE(interval(1), timestamp_t(-1));		// First dive
	QCOMPARE(interval(2), timestamp_t(5400));	// Normal interval
	QCOMPARE(interval(3), timestamp_t(5400));	// Identical start time as the previous dive
	QCOMPARE(interval(4), timestamp_t(0));		// Overlaps with the previous dive
	QCOMPARE(interval(5), timestamp_t(3585));
	QCOMPARE(interval(6), timestamp_t(98800));
	QCOMPARE(interval(7), timestamp_t(6 * SECONDS_PER_DAY));
	QCOMPARE(interval(8), timestamp_t(4000000000LL));
}

// A dive that is not (yet) part of the log gives the same result as the timestamp-based calculation.
void TestSurfaceIntervalColumn::testLookupForDiveNotInTable()
{
	dive d;
	d.number = 100;
	d.when = START_D + 100000;
	QCOMPARE(divelog.dives.get_surface_interval_before(d), divelog.dives.get_surface_interval(d.when));
	d.when = START_A - 1;
	QCOMPARE(divelog.dives.get_surface_interval_before(d), timestamp_t(-1));
}

// Column visibility, width and order are saved by logical column index.
// Therefore, the new column must be appended after all existing ones.
void TestSurfaceIntervalColumn::testColumnPosition()
{
	QCOMPARE(int(DiveTripModelBase::DIVEMODE), 20);
	QCOMPARE(int(DiveTripModelBase::SURFACE_INTERVAL), 21);
	QCOMPARE(int(DiveTripModelBase::SURFACE_GF), 22);
	QCOMPARE(int(DiveTripModelBase::COLUMNS), 23);

	DiveTripModelList listModel;
	DiveTripModelBase &model = listModel;
	QCOMPARE(model.columnCount(QModelIndex()), int(DiveTripModelBase::COLUMNS));
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_INTERVAL, Qt::Horizontal, Qt::DisplayRole).toString(),
		 QString("Surface interval"));
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_INTERVAL, Qt::Horizontal, Qt::ToolTipRole).toString(),
		 QString("Surface interval before this dive"));
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_INTERVAL, Qt::Horizontal, Qt::TextAlignmentRole).toInt(),
		 int(Qt::AlignRight | Qt::AlignVCenter));
}

void TestSurfaceIntervalColumn::testListModelCells()
{
	// The units must be shown independently of the setting for the table
	for (bool showUnits: { true, false }) {
		prefs.units.show_units_table = showUnits;
		DiveTripModelList listModel;
		DiveTripModelBase &model = listModel;
		QCOMPARE(model.rowCount(), 8);

		auto text = [&](int number) {
			int row = rowOf(model, diveByNumber(number));
			return row >= 0 ? surfaceInterval(model, row) : QStringLiteral("not shown");
		};
		QCOMPARE(text(1), QString());
		QCOMPARE(text(2), QString("1h 30min"));
		QCOMPARE(text(3), QString("1h 30min"));
		QCOMPARE(text(4), QString("0h 0min"));
		QCOMPARE(text(5), QString("1h 0min"));		// 59min 45s is rounded up
		QCOMPARE(text(6), QString("1d 3h 27min"));
		QCOMPARE(text(7), QString("more than 4 days"));
		QCOMPARE(text(8), QString("more than 4 days"));

		QModelIndex idx = model.index(rowOf(model, diveByNumber(2)), DiveTripModelBase::SURFACE_INTERVAL);
		QCOMPARE(idx.data(Qt::ToolTipRole).toString(), QString("Surface interval before this dive"));
		QCOMPARE(idx.data(Qt::TextAlignmentRole).toInt(), int(Qt::AlignRight | Qt::AlignVCenter));
	}
}

// Put the given dive into a new trip. The dive must not share its start time with another dive,
// as this would change the order of the dives in the log.
static dive_trip *putInTrip(dive *d)
{
	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(d);
	return trip;
}

// The tree model shows the same values for dives that are inside of trips
// and for dives outside of trips. Trip rows show nothing in this column.
void TestSurfaceIntervalColumn::testTreeModelCells()
{
	putInTrip(diveByNumber(5));

	DiveTripModelTree treeModel;
	DiveTripModelBase &model = treeModel;
	QCOMPARE(model.rowCount(), 8 - 1 + 1); // Seven top-level dives and one trip

	int row2 = rowOf(model, diveByNumber(2));
	QVERIFY(row2 >= 0);
	QCOMPARE(surfaceInterval(model, row2), QString("1h 30min"));
	int row4 = rowOf(model, diveByNumber(4));
	QVERIFY(row4 >= 0);
	QCOMPARE(surfaceInterval(model, row4), QString("0h 0min"));
	QCOMPARE(surfaceInterval(model, rowOf(model, diveByNumber(1))), QString());

	bool foundTrip = false;
	for (int row = 0; row < model.rowCount(); ++row) {
		QModelIndex idx = model.index(row, 0);
		if (!idx.data(DiveTripModelBase::IS_TRIP_ROLE).toBool())
			continue;
		foundTrip = true;
		QCOMPARE(surfaceInterval(model, row), QString());
		QCOMPARE(model.rowCount(idx), 1);
		QModelIndex child = model.index(0, DiveTripModelBase::SURFACE_INTERVAL, idx);
		QCOMPARE(child.data(Qt::DisplayRole).toString(), QString("1h 0min"));
		QCOMPARE(child.data(Qt::ToolTipRole).toString(), QString("Surface interval before this dive"));
	}
	QVERIFY(foundTrip);
}

// The interval is calculated against the whole dive log, even if the previous dive is hidden.
void TestSurfaceIntervalColumn::testWholeLogWithHiddenDive()
{
	diveByNumber(4)->invalid = true;
	prefs.display_invalid_dives = false;

	DiveTripModelList listModel;
	DiveTripModelBase &model = listModel;
	QCOMPARE(model.rowCount(), 7);
	QCOMPARE(rowOf(model, diveByNumber(4)), -1);

	// Dive 5 still shows the interval to the hidden dive 4 (and not the one to dive 3)
	int row = rowOf(model, diveByNumber(5));
	QVERIFY(row >= 0);
	QCOMPARE(surfaceInterval(model, row), QString("1h 0min"));
}

void TestSurfaceIntervalColumn::testListSorting()
{
	DiveTripModelList listModel;
	DiveTripModelBase &model = listModel;
	auto less = [&](int number1, int number2) {
		return model.lessThan(model.index(rowOf(model, diveByNumber(number1)), DiveTripModelBase::SURFACE_INTERVAL),
				      model.index(rowOf(model, diveByNumber(number2)), DiveTripModelBase::SURFACE_INTERVAL));
	};
	// No interval (-1) sorts before an interval of 0 seconds
	QVERIFY(less(1, 4));
	QVERIFY(!less(4, 1));
	QVERIFY(less(4, 5));
	QVERIFY(less(5, 2));
	QVERIFY(less(2, 6));
	QVERIFY(less(6, 7));
	// An interval that doesn't fit into 32 bits must not wrap around
	QVERIFY(less(7, 8));
	QVERIFY(!less(8, 7));
	QVERIFY(!less(8, 1));
	// Dives with equal intervals are ordered chronologically descending
	QVERIFY(less(3, 2));
	QVERIFY(!less(2, 3));
	// ... and a dive is not less than itself
	QVERIFY(!less(2, 2));
}

// Does one of the dataChanged() signals cover the given cell?
static bool cellChanged(const QSignalSpy &spy, const QModelIndex &cell)
{
	for (const QList<QVariant> &args: spy) {
		QModelIndex from = args[0].value<QModelIndex>();
		QModelIndex to = args[1].value<QModelIndex>();
		if (from.parent() == cell.parent() &&
		    from.row() <= cell.row() && cell.row() <= to.row() &&
		    from.column() <= cell.column() && cell.column() <= to.column())
			return true;
	}
	return false;
}

// Changing a dive changes the interval of the next dive in the log. Check that the model
// informs the views about the cell of that dive and that the new value is reported.
void TestSurfaceIntervalColumn::testSuccessorRefreshList()
{
	DiveTripModelList listModel;
	DiveTripModelBase &model = listModel;
	dive *d4 = diveByNumber(4);
	int row4 = rowOf(model, d4);
	int row5 = rowOf(model, diveByNumber(5));
	int row7 = rowOf(model, diveByNumber(7));
	QCOMPARE(surfaceInterval(model, row5), QString("1h 0min"));

	QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
	d4->duration.seconds = 1200;	// Dive 4 now ends 600 s later: the interval of dive 5 is 2985 s
	emit diveListNotifier.divesChanged(QVector<dive *> { d4 }, DiveField::DURATION);

	QVERIFY(cellChanged(spy, model.index(row5, DiveTripModelBase::SURFACE_INTERVAL)));
	QVERIFY(cellChanged(spy, model.index(row4, DiveTripModelBase::DURATION)));
	// No other dive is affected
	QVERIFY(!cellChanged(spy, model.index(row7, DiveTripModelBase::SURFACE_INTERVAL)));
	QCOMPARE(surfaceInterval(model, row5), QString("0h 50min"));

	// Dives 2 and 3 start at the same time, so both have dive 1 as predecessor.
	// Changing dive 1 changes the interval of both of them.
	spy.clear();
	diveByNumber(1)->duration.seconds = 600;
	emit diveListNotifier.divesChanged(QVector<dive *> { diveByNumber(1) }, DiveField::DURATION);
	QVERIFY(cellChanged(spy, model.index(rowOf(model, diveByNumber(2)), DiveTripModelBase::SURFACE_INTERVAL)));
	QVERIFY(cellChanged(spy, model.index(rowOf(model, diveByNumber(3)), DiveTripModelBase::SURFACE_INTERVAL)));
	QCOMPARE(surfaceInterval(model, rowOf(model, diveByNumber(2))), QString("1h 50min"));
	QCOMPARE(surfaceInterval(model, rowOf(model, diveByNumber(3))), QString("1h 50min"));
	diveByNumber(1)->duration.seconds = 1800;

	// If the successor is part of the change, its row is updated as a whole anyway.
	spy.clear();
	emit diveListNotifier.divesChanged(QVector<dive *> { d4, diveByNumber(5) }, DiveField::DURATION);
	QVERIFY(cellChanged(spy, model.index(row5, DiveTripModelBase::SURFACE_INTERVAL)));

	// If the successor is hidden, it has no row that could be refreshed.
	// This must not result in signals for invalid cells.
	dive *d7 = diveByNumber(7);
	d7->invalid = true;
	prefs.display_invalid_dives = false;
	emit diveListNotifier.divesChanged(QVector<dive *> { d7 }, DiveField::INVALID);
	QVERIFY(d7->hidden_by_filter);
	QCOMPARE(rowOf(model, d7), -1);
	spy.clear();
	emit diveListNotifier.divesChanged(QVector<dive *> { diveByNumber(6) }, DiveField::DURATION);
	QVERIFY(spy.count() >= 1); // The GF cache refresh also notifies every visible row.
	for (const auto &args: spy) {
		QVERIFY(args[0].value<QModelIndex>().isValid());
		QVERIFY(args[1].value<QModelIndex>().isValid());
	}
}

void TestSurfaceIntervalColumn::testSuccessorRefreshTree()
{
	// The successor of the changed dive is inside a trip
	dive *d4 = diveByNumber(4);
	putInTrip(diveByNumber(5));

	DiveTripModelTree treeModel;
	DiveTripModelBase &model = treeModel;
	int row4 = rowOf(model, d4);
	QVERIFY(row4 >= 0);
	QModelIndex tripIdx;
	for (int row = 0; row < model.rowCount(); ++row) {
		if (model.index(row, 0).data(DiveTripModelBase::IS_TRIP_ROLE).toBool())
			tripIdx = model.index(row, 0);
	}
	QVERIFY(tripIdx.isValid());
	QModelIndex cell = model.index(0, DiveTripModelBase::SURFACE_INTERVAL, tripIdx);
	QCOMPARE(cell.data(Qt::DisplayRole).toString(), QString("1h 0min"));

	QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
	d4->duration.seconds = 1200;
	emit diveListNotifier.divesChanged(QVector<dive *> { d4 }, DiveField::DURATION);

	QVERIFY(cellChanged(spy, cell));
	QVERIFY(cellChanged(spy, model.index(row4, DiveTripModelBase::DURATION)));
	QCOMPARE(cell.data(Qt::DisplayRole).toString(), QString("0h 50min"));

	// A successor outside of a trip is refreshed, too
	spy.clear();
	dive *d6 = diveByNumber(6);
	dive *d7 = diveByNumber(7);
	d6->duration.seconds = 1200;
	emit diveListNotifier.divesChanged(QVector<dive *> { d6 }, DiveField::DURATION);
	QVERIFY(cellChanged(spy, model.index(rowOf(model, d7), DiveTripModelBase::SURFACE_INTERVAL)));
}

// Cell of the surface interval of the given dive, wherever it is in the model.
static QModelIndex surfaceIntervalCell(const DiveTripModelBase &model, const dive *d, const QModelIndex &parent = QModelIndex())
{
	for (int row = 0; row < model.rowCount(parent); ++row) {
		QModelIndex idx = model.index(row, DiveTripModelBase::SURFACE_INTERVAL, parent);
		if (idx.data(DiveTripModelBase::DIVE_ROLE).value<dive *>() == d)
			return idx;
		QModelIndex child = surfaceIntervalCell(model, d, model.index(row, 0, parent));
		if (child.isValid())
			return child;
	}
	return QModelIndex();
}

// Remove a dive from the log and tell the model about it, as the delete command does.
static std::unique_ptr<dive> deleteDive(dive *d)
{
	auto removed = divelog.dives.unregister_dive(divelog.dives.get_idx(d));
	emit diveListNotifier.divesDeleted(nullptr, false, QVector<dive *> { d });
	return removed;
}

// Move a dive in the log and tell the model about it, as the commands that change the
// start time do: first about the time change, then about the changed dive.
static void moveDive(dive *d, timestamp_t delta)
{
	d->when += delta;
	divelog.dives.sort();
	emit diveListNotifier.divesTimeChanged(delta, QVector<dive *> { d });
	emit diveListNotifier.divesChanged(QVector<dive *> { d }, DiveField::DATETIME);
}

// Adding, deleting or moving a dive changes the interval of the dive that follows it,
// even if that dive does not get a row inserted or removed.
// The dives 4 and 5 start at 17500 and 21685; dive 3 ends at 17800.
static void checkAddDeleteMove(DiveTripModelBase &model)
{
	dive *d4 = diveByNumber(4);
	dive *d5 = diveByNumber(5);
	QCOMPARE(surfaceIntervalCell(model, d5).data(Qt::DisplayRole).toString(), QString("1h 0min"));

	// Delete dive 4: dive 5 now follows dive 3 (and not dive 4, which it overlapped with)
	QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
	auto removed = deleteDive(d4);
	QVERIFY(cellChanged(spy, surfaceIntervalCell(model, d5)));
	QCOMPARE(surfaceIntervalCell(model, d5).data(Qt::DisplayRole).toString(), QString("1h 5min"));

	// Add it back
	spy.clear();
	d4 = divelog.dives.register_dive(std::move(removed));
	DiveFilter::instance()->update(QVector<dive *> { d4 });
	emit diveListNotifier.divesAdded(nullptr, false, QVector<dive *> { d4 });
	QVERIFY(cellChanged(spy, surfaceIntervalCell(model, d5)));
	QCOMPARE(surfaceIntervalCell(model, d5).data(Qt::DisplayRole).toString(), QString("1h 0min"));

	// Add a dive between dive 4 and 5: it is now the predecessor of dive 5
	spy.clear();
	dive *added = addDive(100, START_D + 1000, 300);
	DiveFilter::instance()->update(QVector<dive *> { added });
	emit diveListNotifier.divesAdded(nullptr, false, QVector<dive *> { added });
	QVERIFY(cellChanged(spy, surfaceIntervalCell(model, d5)));
	QCOMPARE(surfaceIntervalCell(model, d5).data(Qt::DisplayRole).toString(), QString("0h 48min"));
	QCOMPARE(surfaceIntervalCell(model, added).data(Qt::DisplayRole).toString(), QString("0h 7min"));
	removed = deleteDive(added);

	// Move dive 4 past dive 5: dive 5, which has been its successor, now follows dive 3,
	// and dive 6 now follows dive 4.
	spy.clear();
	dive *d6 = diveByNumber(6);
	moveDive(d4, 10000);
	QVERIFY(cellChanged(spy, surfaceIntervalCell(model, d5)));
	QVERIFY(cellChanged(spy, surfaceIntervalCell(model, d6)));
	QCOMPARE(surfaceIntervalCell(model, d5).data(Qt::DisplayRole).toString(), QString("1h 5min"));
	QCOMPARE(surfaceIntervalCell(model, d6).data(Qt::DisplayRole).toString(), QString("1d 2h 0min"));
}

void TestSurfaceIntervalColumn::testSuccessorRefreshAddDeleteMoveList()
{
	DiveTripModelList listModel;
	checkAddDeleteMove(listModel);
}

void TestSurfaceIntervalColumn::testSuccessorRefreshAddDeleteMoveTree()
{
	// Dive 5, the successor of the changed dives, is inside of a trip
	putInTrip(diveByNumber(5));
	DiveTripModelTree treeModel;
	checkAddDeleteMove(treeModel);
}

// Stubs for symbols referenced (but never exercised) by other desktop qt-models
// source files that end up linked into this test binary alongside the dive list
// models, because they all share subsurface_models_desktop.
// See testcylindersmodelsensorrefresh.cpp for the same pattern.
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

QTEST_GUILESS_MAIN(TestSurfaceIntervalColumn)
