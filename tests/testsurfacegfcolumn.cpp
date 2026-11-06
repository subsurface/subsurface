// SPDX-License-Identifier: GPL-2.0
#include "testsurfacegfcolumn.h"
#include "core/divefilter.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/gas.h"
#include "core/pref.h"
#include "core/sample.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefGeneral.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/trip.h"
#include "qt-models/divetripmodel.h"
#include "commands/command.h"
#if defined(MAP_SUPPORT)
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif
#include <QSignalSpy>

static dive *addDive(int number, timestamp_t when, depth_t depth)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;
	if (depth.mm) {
		auto *cyl = d->get_or_create_cylinder(0);
		cyl->gasmix = gasmix_air;
		cyl->type.size = 15_l;
		cyl->type.workingpressure = 200_bar;
		reset_cylinders(d.get(), true);
		auto &dc = d->dcs[0];
		dc.when = when;
		dc.surface_pressure = 1013_mbar;
		dc.divemode = OC;
		const std::pair<int, int> samples[] = { {0, 0}, {120, depth.mm}, {540, depth.mm}, {660, 0} };
		for (auto [seconds, mm]: samples) {
			auto *sample = prepare_sample(&dc);
			sample->time.seconds = seconds;
			sample->depth.mm = mm;
		}
		dc.duration.seconds = 660;
		divelog.dives.fixup_dive(*d);
	}
	return divelog.dives.register_dive(std::move(d));
}

static QModelIndex cell(const DiveTripModelBase &model, const dive *d)
{
	for (int row = 0; row < model.rowCount(); ++row) {
		QModelIndex idx = model.index(row, DiveTripModelBase::SURFACE_GF);
		if (idx.data(DiveTripModelBase::DIVE_ROLE).value<dive *>() == d)
			return idx;
	}
	return {};
}

void TestSurfaceGfColumn::init()
{
	prefs = default_prefs;
	prefs.units.show_units_table = false;
	divelog.clear();
	DiveFilter::instance()->reset();
}

void TestSurfaceGfColumn::cleanup()
{
	divelog.clear();
	DiveFilter::instance()->reset();
}

void TestSurfaceGfColumn::testDisplaySortAndRefresh()
{
	dive *deep = addDive(1, 1000000000, 30_m);
	dive *shallow = addDive(2, 1000001000, 10_m);
	dive *empty = addDive(3, 1000002000, 0_m);
	DiveTripModelList model;
	DiveTripModelBase &base = model;
	QCOMPARE(int(DiveTripModelBase::SURFACE_GF), 22);
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_GF, Qt::Horizontal).toString(), QString("Surface GF"));
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_GF, Qt::Horizontal, Qt::ToolTipRole).toString(), QString("Surface GF at the end of the dive"));
	QModelIndex deepCell = cell(model, deep);
	QModelIndex shallowCell = cell(model, shallow);
	QModelIndex emptyCell = cell(model, empty);
	QVERIFY(deepCell.isValid() && shallowCell.isValid() && emptyCell.isValid());
	QCOMPARE(deepCell.data(Qt::TextAlignmentRole).toInt(), int(Qt::AlignRight | Qt::AlignVCenter));
	QVERIFY(!emptyCell.data(Qt::DisplayRole).isValid());
	auto gfDeep = divelog.dives.end_surface_gf(*deep);
	auto gfShallow = divelog.dives.end_surface_gf(*shallow);
	QVERIFY(gfDeep && gfShallow);
	prefs.units.show_units_table = false;
	QCOMPARE(deepCell.data().toInt(), qRound(*gfDeep));
	QCOMPARE(shallowCell.data().toInt(), qRound(*gfShallow));
	prefs.units.show_units_table = true;
	QCOMPARE(deepCell.data().toString(), QString("%1%").arg(qRound(*gfDeep)));
	QCOMPARE(base.lessThan(deepCell, emptyCell), true);
	QVERIFY(!base.lessThan(emptyCell, emptyCell));
	QCOMPARE(base.lessThan(emptyCell, deepCell), false);
	QCOMPARE(base.lessThan(shallowCell, emptyCell), true);
	QCOMPARE(base.lessThan(deepCell, shallowCell), *gfDeep < *gfShallow || (*gfDeep == *gfShallow && deepCell.row() > shallowCell.row()));
	QCOMPARE(base.lessThan(shallowCell, deepCell), *gfShallow < *gfDeep || (*gfDeep == *gfShallow && shallowCell.row() > deepCell.row()));

	QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
	deep->dcs[0].samples[1].depth = 20_m;
	deep->dcs[0].samples[2].depth = 20_m;
	emit diveListNotifier.divesChanged(QVector<dive *> { deep }, DiveField::DEPTH);
	auto updated = divelog.dives.end_surface_gf(*deep);
	QVERIFY(updated && *updated != *gfDeep);
	prefs.units.show_units_table = false;
	QCOMPARE(deepCell.data().toInt(), qRound(*updated));
	bool refreshed = false;
	for (const auto &args: spy) {
		QModelIndex first = args[0].value<QModelIndex>();
		QModelIndex last = args[1].value<QModelIndex>();
		if (first.row() <= shallowCell.row() && last.row() >= shallowCell.row() &&
		    first.column() <= DiveTripModelBase::SURFACE_GF && last.column() >= DiveTripModelBase::SURFACE_GF)
			refreshed = true;
	}
	QVERIFY(refreshed);
	int originalRatio = prefs.pscr_ratio;
	int originalConsumption = prefs.o2consumption;
	int originalBottomsac = prefs.bottomsac;

	// With no PSCR dive logged, none of pscr_ratio/o2consumption/bottomsac
	// can affect any computed Surface GF, so changing one must not trigger
	// a refresh at all.
	spy.clear();
	qPrefGeneral::set_pscr_ratio(originalRatio + 1);
	QVERIFY(spy.isEmpty());
	qPrefGeneral::set_pscr_ratio(originalRatio);

	// Once a PSCR dive is logged, pscr_o2() (core/gas.cpp) computes its O2
	// partial pressure from all three of these preferences together, so
	// all three must invalidate the cache.
	deep->dcs[0].divemode = PSCR;
	spy.clear();
	qPrefGeneral::set_pscr_ratio(originalRatio + 1);
	QVERIFY(!spy.isEmpty());
	spy.clear();
	qPrefGeneral::set_o2consumption(originalConsumption + 1);
	QVERIFY(!spy.isEmpty());
	spy.clear();
	qPrefDivePlanner::set_bottomsac(originalBottomsac + 1000);
	QVERIFY(!spy.isEmpty());
	qPrefGeneral::set_pscr_ratio(originalRatio);
	qPrefGeneral::set_o2consumption(originalConsumption);
	qPrefDivePlanner::set_bottomsac(originalBottomsac);
	deep->dcs[0].divemode = OC;

	// A dive computer declared OC overall, but with a mid-dive "modechange"
	// event switching into PSCR, must be detected too: divemode_loop()
	// applies such events regardless of the dive computer's own declared
	// divemode.
	add_event(&deep->dcs[0], 120, SAMPLE_EVENT_BOOKMARK, 0, PSCR, "modechange");
	spy.clear();
	qPrefGeneral::set_pscr_ratio(originalRatio + 1);
	QVERIFY(!spy.isEmpty());
	qPrefGeneral::set_pscr_ratio(originalRatio);
}

void TestSurfaceGfColumn::testHiddenDiveAndTree()
{
	dive *first = addDive(1, 1000000000, 30_m);
	dive *second = addDive(2, 1000001000, 10_m);
	auto expected = divelog.dives.end_surface_gf(*second);
	QVERIFY(expected);
	first->invalid = true;
	prefs.display_invalid_dives = false;
	{
		DiveTripModelList list;
		QVERIFY(!cell(list, first).isValid());
		QCOMPARE(cell(list, second).data().toInt(), qRound(*expected));
	}

	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(second);
	DiveTripModelTree tree;
	DiveTripModelBase &base = tree;
	bool found = false;
	for (int row = 0; row < base.rowCount(); ++row) {
		QModelIndex parent = base.index(row, 0);
		if (!parent.data(DiveTripModelBase::IS_TRIP_ROLE).toBool())
			continue;
		QModelIndex child = base.index(0, DiveTripModelBase::SURFACE_GF, parent);
		QCOMPARE(child.data().toInt(), qRound(*divelog.dives.end_surface_gf(*second)));
		QVERIFY(!base.index(row, DiveTripModelBase::SURFACE_GF).data().isValid());
		found = true;
	}
	QVERIFY(found);
}

void TestSurfaceGfColumn::testAddDeleteAndTimeShift()
{
	dive *first = addDive(1, 1000000000, 30_m);
	dive *second = addDive(2, 1000001000, 10_m);
	DiveTripModelList model;
	QModelIndex secondCell = cell(model, second);
	QVERIFY(secondCell.isValid());
	std::optional<double> expected = divelog.dives.end_surface_gf(*second);
	QVERIFY(expected);
	// Populate the cache before changing the log, and confirm it starts out
	// correct, so that the comparisons below after each event are actually
	// meaningful.
	QCOMPARE(secondCell.data().toInt(), qRound(*expected));
	QSignalSpy spy(&model, &QAbstractItemModel::dataChanged);
	auto changedSecond = [&] {
		for (const auto &args: spy) {
			QModelIndex from = args[0].value<QModelIndex>();
			QModelIndex to = args[1].value<QModelIndex>();
			if (from.row() <= secondCell.row() && to.row() >= secondCell.row() &&
			    from.column() <= DiveTripModelBase::SURFACE_GF && to.column() >= DiveTripModelBase::SURFACE_GF)
				return true;
		}
		return false;
	};
	// An added or removed predecessor affects a later row, not just the
	// edited row. Use a sampled predecessor (rather than an empty dive) and
	// check the cell's actual value against a fresh calculation after each
	// event, not just whether dataChanged was emitted: a refresh that
	// emits dataChanged without actually recalculating the cached value
	// would otherwise still pass.
	dive *added = addDive(3, first->when - 2000, 20_m);
	emit diveListNotifier.divesAdded(nullptr, false, QVector<dive *> { added });
	secondCell = cell(model, second);
	QVERIFY(changedSecond());
	std::optional<double> expectedWithPredecessor = divelog.dives.end_surface_gf(*second);
	QVERIFY(expectedWithPredecessor);
	QVERIFY(qRound(*expectedWithPredecessor) != qRound(*expected));
	QCOMPARE(secondCell.data().toInt(), qRound(*expectedWithPredecessor));

	spy.clear();
	auto removed = divelog.dives.unregister_dive(divelog.dives.get_idx(added));
	emit diveListNotifier.divesDeleted(nullptr, false, QVector<dive *> { added });
	secondCell = cell(model, second);
	QVERIFY(changedSecond());
	QCOMPARE(secondCell.data().toInt(), qRound(*expected));

	spy.clear();
	first->when -= 60;
	divelog.dives.sort();
	emit diveListNotifier.divesTimeChanged(-60, QVector<dive *> { first });
	secondCell = cell(model, second);
	QVERIFY(changedSecond());
	std::optional<double> expectedAfterShift = divelog.dives.end_surface_gf(*second);
	QVERIFY(expectedAfterShift);
	QVERIFY(qRound(*expectedAfterShift) != qRound(*expected));
	QCOMPARE(secondCell.data().toInt(), qRound(*expectedAfterShift));
}

void TestSurfaceGfColumn::testDecoModelTooltip()
{
	dive *d = addDive(1, 1000000000, 30_m);
	DiveTripModelList model;
	DiveTripModelBase &base = model;
	QModelIndex gfCell = cell(model, d);
	QVERIFY(gfCell.isValid());
	const QString description = model.headerData(DiveTripModelBase::SURFACE_GF, Qt::Horizontal, Qt::ToolTipRole).toString();
	QCOMPARE(gfCell.data(Qt::ToolTipRole).toString(), description);
	QModelIndex depthCell = base.index(gfCell.row(), DiveTripModelBase::DEPTH);
	const QString depthTooltip = depthCell.data(Qt::ToolTipRole).toString();
	QCOMPARE(depthTooltip, model.headerData(DiveTripModelBase::DEPTH, Qt::Horizontal, Qt::ToolTipRole).toString());

	add_extra_data(&d->dcs[0], "GF Low", "30");
	QCOMPARE(gfCell.data(Qt::ToolTipRole).toString(), description);
	add_extra_data(&d->dcs[0], "GF High", "70");
	QCOMPARE(gfCell.data(Qt::ToolTipRole).toString(), description + "\nDive computer deco model: GF 30/70");
	add_extra_data(&d->dcs[0], "Deco model", "Buhlmann ZHL-16C 40/80");
	QCOMPARE(gfCell.data(Qt::ToolTipRole).toString(), description + "\nDive computer deco model: Buhlmann ZHL-16C 40/80");
	QCOMPARE(depthCell.data(Qt::ToolTipRole).toString(), depthTooltip);
	QCOMPARE(model.headerData(DiveTripModelBase::SURFACE_GF, Qt::Horizontal, Qt::ToolTipRole).toString(), description);

	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(d);
	DiveTripModelTree tree;
	DiveTripModelBase &treeBase = tree;
	for (int row = 0; row < treeBase.rowCount(); ++row) {
		QModelIndex parent = treeBase.index(row, 0);
		if (!parent.data(DiveTripModelBase::IS_TRIP_ROLE).toBool())
			continue;
		QModelIndex tripCell = treeBase.index(row, DiveTripModelBase::SURFACE_GF);
		QVERIFY(!tripCell.data(Qt::ToolTipRole).isValid());
		QCOMPARE(treeBase.index(0, DiveTripModelBase::SURFACE_GF, parent).data(Qt::ToolTipRole).toString(),
			 description + "\nDive computer deco model: Buhlmann ZHL-16C 40/80");
		return;
	}
	QFAIL("Trip row not found");
}

// The desktop models archive also references these unrelated commands and widgets.
// None is exercised by the dive-list model tests.
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
MapWidget *MapWidget::instance() { return m_instance; }
bool MapWidget::editMode() const { return false; }
void MapWidget::reload() {}
void MapWidget::setSelected(std::vector<dive_site *>) {}
MainWindow *MainWindow::instance() { return nullptr; }
#endif

QTEST_GUILESS_MAIN(TestSurfaceGfColumn)
