// SPDX-License-Identifier: GPL-2.0
#include "testpastedives.h"
#include "commands/command.h"
#include "core/dive.h"
#include "core/divefilter.h"
#include "core/divelog.h"
#include "core/pref.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/trip.h"
#include "qt-models/divetripmodel.h"
#if defined(MAP_SUPPORT)
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

#include <algorithm>
#include <map>
#include <memory>
#include <QAction>
#include <QSignalSpy>

namespace {

// The dives of the test log, identified by name. The times are chosen such that
// there is a trip with three dives, and dives before, between and after the trip:
//
//	trip: A (1000), B (2000), C (3000)
//	no trip: F (1500), G (5000), E (10000)
//
// In the tree model, this yields the top-level order trip(A), F, G, E.
//
// createTwoTripLog() has two trips instead, and no dives outside of them:
//
//	trip: A (1000), B (2000), C (3000)
//	second trip: X (5000), Y (6000)
struct TestLog {
	std::map<std::string, dive *> byName;
	dive_trip *trip = nullptr;
	dive_trip *secondTrip = nullptr; // Only set by createTwoTripLog()

	dive *operator[](const std::string &name) const { return byName.at(name); }
	std::string nameOf(const dive *d) const
	{
		for (auto &[name, ptr]: byName) {
			if (ptr == d)
				return name;
		}
		return "?";
	}
	QStringList names(const std::vector<dive *> &dives) const
	{
		QStringList res;
		for (const dive *d: dives)
			res << QString::fromStdString(nameOf(d));
		return res;
	}
};

TestLog createLog()
{
	TestLog res;
	auto tripPtr = std::make_unique<dive_trip>();
	res.trip = tripPtr.get();

	auto add = [&res](const std::string &name, timestamp_t when, bool inTrip) {
		auto d = std::make_unique<dive>();
		d->when = when;
		d->notes = name;
		if (inTrip)
			res.trip->add_dive(d.get());
		res.byName[name] = divelog.dives.register_dive(std::move(d));
	};
	add("A", 1000, true);
	add("B", 2000, true);
	add("C", 3000, true);
	add("F", 1500, false);
	add("G", 5000, false);
	add("E", 10000, false);
	divelog.trips.put(std::move(tripPtr));
	DiveFilter::instance()->reset(); // Registered dives start out hidden, make them visible
	return res;
}

// Two trips, to test dives that move across the boundary between them.
TestLog createTwoTripLog()
{
	TestLog res;
	auto tripPtr = std::make_unique<dive_trip>();
	auto secondTripPtr = std::make_unique<dive_trip>();
	res.trip = tripPtr.get();
	res.secondTrip = secondTripPtr.get();

	auto add = [&res](const std::string &name, timestamp_t when, dive_trip *trip) {
		auto d = std::make_unique<dive>();
		d->when = when;
		d->notes = name;
		trip->add_dive(d.get());
		res.byName[name] = divelog.dives.register_dive(std::move(d));
	};
	add("A", 1000, res.trip);
	add("B", 2000, res.trip);
	add("C", 3000, res.trip);
	add("X", 5000, res.secondTrip);
	add("Y", 6000, res.secondTrip);
	divelog.trips.put(std::move(tripPtr));
	divelog.trips.put(std::move(secondTripPtr));
	DiveFilter::instance()->reset(); // Registered dives start out hidden, make them visible
	return res;
}

// Dive list as seen by the core: the dive table and the dive list of the trip.
QStringList coreOrder(const TestLog &log)
{
	QStringList res;
	for (auto &d: divelog.dives)
		res << QString::fromStdString(log.nameOf(d.get()));
	return res;
}

QStringList tripOrder(const TestLog &log)
{
	return log.names(log.trip->dives);
}

QStringList secondTripOrder(const TestLog &log)
{
	return log.names(log.secondTrip->dives);
}

// Check that the core data structures are sorted and consistent
void verifyCore(const TestLog &log, const QStringList &expectedDives, const QStringList &expectedTrip,
		const QStringList &expectedSecondTrip = {})
{
	QCOMPARE(coreOrder(log), expectedDives);
	QCOMPARE(tripOrder(log), expectedTrip);
	if (log.secondTrip) {
		QCOMPARE(secondTripOrder(log), expectedSecondTrip);
		QVERIFY(std::is_sorted(log.secondTrip->dives.begin(), log.secondTrip->dives.end(), dive_less_than_ptr));
	}
	QVERIFY(std::is_sorted(divelog.dives.begin(), divelog.dives.end(),
			       [](auto &a, auto &b) { return comp_dives(*a, *b) < 0; }));
	QVERIFY(std::is_sorted(divelog.trips.begin(), divelog.trips.end(),
			       [](auto &a, auto &b) { return comp_trips(*a, *b) < 0; }));
	QVERIFY(std::is_sorted(log.trip->dives.begin(), log.trip->dives.end(), dive_less_than_ptr));
	for (int i = 0; i < expectedDives.size(); ++i)
		QCOMPARE(divelog.dives.get_idx(log[expectedDives[i].toStdString()]), static_cast<size_t>(i));
}

// The tree model has one row per trip or top-level dive. Describe the rows
// as "F" for a dive and "T[A,B,C]" for a trip.
QStringList modelRows(const TestLog &log, const DiveTripModelBase &model)
{
	QStringList res;
	for (int row = 0; row < model.rowCount(); ++row) {
		QModelIndex idx = model.index(row, 0);
		if (model.data(idx, DiveTripModelBase::IS_TRIP_ROLE).toBool()) {
			QStringList children;
			for (int child = 0; child < model.rowCount(idx); ++child) {
				QModelIndex cidx = model.index(child, 0, idx);
				children << QString::fromStdString(log.nameOf(model.data(cidx, DiveTripModelBase::DIVE_ROLE).value<dive *>()));
			}
			res << QStringLiteral("T[%1]").arg(children.join(','));
		} else {
			res << QString::fromStdString(log.nameOf(model.data(idx, DiveTripModelBase::DIVE_ROLE).value<dive *>()));
		}
	}
	return res;
}

// Rows of the list model: simply the dives.
QStringList listRows(const TestLog &log, const DiveTripModelBase &model)
{
	QStringList res;
	for (int row = 0; row < model.rowCount(); ++row)
		res << QString::fromStdString(log.nameOf(model.data(model.index(row, 0), DiveTripModelBase::DIVE_ROLE).value<dive *>()));
	return res;
}

// Records divesTimeChanged() signals.
struct TimeChange {
	timestamp_t delta;
	QVector<dive *> dives;
};

QList<TimeChange> timeChanges(const QSignalSpy &spy)
{
	QList<TimeChange> res;
	for (const QList<QVariant> &args: spy)
		res.push_back({ args[0].value<timestamp_t>(), args[1].value<QVector<dive *>>() });
	return res;
}

void pasteWhen(timestamp_t when, const std::vector<dive *> &selection)
{
	setSelection(selection, selection[0], -1);
	dive_paste_data data;
	data.when = when;
	Command::pasteDives(data);
}

struct Expected {
	QStringList dives;
	QStringList trip;
	QStringList treeRows;
	QStringList secondTrip; // Only used with createTwoTripLog()
};

// Run: initial state -> paste -> undo -> redo -> undo -> redo, checking core data and model each time.
void runScenario(const std::vector<std::string> &targets, timestamp_t when,
		 const Expected &before, const Expected &after,
		 const std::vector<std::pair<std::string, timestamp_t>> &expectedDeltas,
		 TestLog (*create)() = createLog)
{
	divelog.clear();
	TestLog log = create();

	// Only one dive list model may exist at any time, because they share the filter state.
	for (int layout = 0; layout < 2; ++layout) {
		if (layout > 0) {
			divelog.clear();
			log = create();
		}
		std::unique_ptr<DiveTripModelBase> model;
		if (layout == 0)
			model = std::make_unique<DiveTripModelTree>();
		else
			model = std::make_unique<DiveTripModelList>();
		auto rows = [&] {
			return layout == 0 ? modelRows(log, *model) : listRows(log, *model);
		};
		auto expectedRows = [&](const Expected &e) {
			return layout == 0 ? e.treeRows : e.dives;
		};

		verifyCore(log, before.dives, before.trip, before.secondTrip);
		QCOMPARE(rows(), expectedRows(before));

		std::vector<dive *> selection;
		for (auto &name: targets)
			selection.push_back(log[name]);
		std::vector<timestamp_t> oldWhen;
		for (dive *d: selection)
			oldWhen.push_back(d->when);

		QSignalSpy spy(&diveListNotifier, &DiveListNotifier::divesTimeChanged);
		pasteWhen(when, selection);

		// One signal per dive, each with the delta of that dive.
		auto changes = timeChanges(spy);
		QCOMPARE(changes.size(), static_cast<int>(expectedDeltas.size()));
		for (size_t i = 0; i < expectedDeltas.size(); ++i) {
			QCOMPARE(changes[i].dives.size(), 1);
			QCOMPARE(log.nameOf(changes[i].dives[0]), expectedDeltas[i].first);
			QCOMPARE(changes[i].delta, expectedDeltas[i].second);
		}
		for (size_t i = 0; i < selection.size(); ++i)
			QCOMPARE(selection[i]->when, when);
		verifyCore(log, after.dives, after.trip, after.secondTrip);
		QCOMPARE(rows(), expectedRows(after));

		std::unique_ptr<QAction> undo(Command::undoAction(nullptr));
		std::unique_ptr<QAction> redo(Command::redoAction(nullptr));
		for (int cycle = 0; cycle < 2; ++cycle) {
			spy.clear();
			undo->trigger();
			// The deltas are the negative of the original ones.
			changes = timeChanges(spy);
			QCOMPARE(changes.size(), static_cast<int>(expectedDeltas.size()));
			for (size_t i = 0; i < expectedDeltas.size(); ++i) {
				QCOMPARE(log.nameOf(changes[i].dives[0]), expectedDeltas[i].first);
				QCOMPARE(changes[i].delta, -expectedDeltas[i].second);
			}
			for (size_t i = 0; i < selection.size(); ++i)
				QCOMPARE(selection[i]->when, oldWhen[i]);
			verifyCore(log, before.dives, before.trip, before.secondTrip);
			QCOMPARE(rows(), expectedRows(before));

			spy.clear();
			redo->trigger();
			changes = timeChanges(spy);
			QCOMPARE(changes.size(), static_cast<int>(expectedDeltas.size()));
			for (size_t i = 0; i < expectedDeltas.size(); ++i) {
				QCOMPARE(log.nameOf(changes[i].dives[0]), expectedDeltas[i].first);
				QCOMPARE(changes[i].delta, expectedDeltas[i].second);
			}
			for (size_t i = 0; i < selection.size(); ++i)
				QCOMPARE(selection[i]->when, when);
			verifyCore(log, after.dives, after.trip, after.secondTrip);
			QCOMPARE(rows(), expectedRows(after));
		}

		// Reset: the model is destroyed first, then the log is cleared by the caller.
		model.reset();
		Command::clear();
	}
}

} // namespace

void TestPasteDives::initTestCase()
{
	qRegisterMetaType<timestamp_t>("timestamp_t");
	qRegisterMetaType<QVector<dive *>>("QVector<dive*>");
	Command::init();
}

void TestPasteDives::init()
{
	prefs = default_prefs;
	divelog.clear();
	Command::clear();
}

void TestPasteDives::cleanup()
{
	divelog.clear();
	Command::clear();
}

// Paste a time onto A, which moves it past B inside the trip. The trip
// then starts with B and thus moves behind dive F.
void TestPasteDives::pasteTimeSingleDive()
{
	Expected before { { "A", "F", "B", "C", "G", "E" }, { "A", "B", "C" }, { "T[A,B,C]", "F", "G", "E" } };
	Expected after { { "F", "B", "A", "C", "G", "E" }, { "B", "A", "C" }, { "F", "T[B,A,C]", "G", "E" } };
	runScenario({ "A" }, 2500, before, after, { { "A", 1500 } });
}

// Paste the same time onto two dives, which therefore move by different deltas.
// A moves forward inside its trip, E moves backwards from the end of the log.
void TestPasteDives::pasteTimeMultipleDives()
{
	Expected before { { "A", "F", "B", "C", "G", "E" }, { "A", "B", "C" }, { "T[A,B,C]", "F", "G", "E" } };
	Expected after { { "F", "B", "A", "E", "C", "G" }, { "B", "A", "C" }, { "F", "T[B,A,C]", "E", "G" } };
	runScenario({ "A", "E" }, 2500, before, after, { { "A", 1500 }, { "E", -7500 } });
}

// Paste a time onto A, which moves it past the first dive of the second trip.
// Inside its own trip A moves behind C, so the trip then starts with B. Since
// the trip table is ordered by the first dive of each trip, it must be sorted
// only after the dives of the trip, otherwise the trips end up swapped.
void TestPasteDives::pasteTimeFirstDiveOfTripPastOtherTrip()
{
	Expected before { { "A", "B", "C", "X", "Y" }, { "A", "B", "C" }, { "T[A,B,C]", "T[X,Y]" }, { "X", "Y" } };
	Expected after { { "B", "C", "X", "A", "Y" }, { "B", "C", "A" }, { "T[B,C,A]", "T[X,Y]" }, { "X", "Y" } };
	runScenario({ "A" }, 5500, before, after, { { "A", 4500 } }, createTwoTripLog);
}

// Pasting something else than the time must not report any time change.
void TestPasteDives::pasteWithoutTimeSendsNoTimeSignal()
{
	TestLog log = createLog();
	QSignalSpy timeSpy(&diveListNotifier, &DiveListNotifier::divesTimeChanged);
	int changedCount = 0;
	QObject context;
	QObject::connect(&diveListNotifier, &DiveListNotifier::divesChanged, &context,
			 [&changedCount](const QVector<dive *> &, DiveField) { ++changedCount; });

	setSelection({ log["A"] }, log["A"], -1);
	dive_paste_data data;
	data.notes = "pasted";
	Command::pasteDives(data);

	QCOMPARE(timeSpy.count(), 0);
	QCOMPARE(changedCount, 1);
	QCOMPARE(log["A"]->notes, std::string("pasted"));
	QCOMPARE(log["A"]->when, timestamp_t(1000));
	verifyCore(log, { "A", "F", "B", "C", "G", "E" }, { "A", "B", "C" });

	std::unique_ptr<QAction> undo(Command::undoAction(nullptr));
	undo->trigger();
	QCOMPARE(timeSpy.count(), 0);
	QCOMPARE(log["A"]->notes, std::string("A"));
}

// Stubs for the map widget, which is referenced by subsurface_models_desktop,
// but never used by this test. See testcylindersmodelsensorrefresh.cpp.
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

QTEST_MAIN(TestPasteDives) // QAction (undo and redo) needs a GUI application object
