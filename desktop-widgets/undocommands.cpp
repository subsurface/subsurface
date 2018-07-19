// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/undocommands.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/divelist.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"

// This helper function removes a dive, takes ownership of the dive and adds it to a DiveToAdd structure.
// It is crucial that dives are added in reverse order of deletion, so the the indices are correctly
// set and that the trips are added before they are used!
static DiveToAdd removeDive(struct dive *d)
{
	DiveToAdd res;
	res.idx = get_divenr(d);
	if (res.idx < 0)
		qWarning() << "Deletion of unknown dive!";

	// remove dive from trip - if this is the last dive in the trip
	// remove the whole trip.
	res.trip = unregister_dive_from_trip(d, false);
	if (res.trip && res.trip->nrdives == 0) {
		unregister_trip(res.trip);		// Remove trip from backend
		res.tripToAdd.reset(res.trip);		// Take ownership of trip
	}

	res.dive.reset(unregister_dive(res.idx));	// Remove dive from backend
	return res;
}

// This helper function adds a dive and returns ownership to the backend. It may also add a dive trip.
// It is crucial that dives are added in reverse order of deletion (see comment above)!
// Returns pointer to added dive (which is owned by the backend!)
static dive *addDive(DiveToAdd &d)
{
	if (d.tripToAdd) {
		dive_trip *t = d.tripToAdd.release();	// Give up ownership of trip
		insert_trip(&t);			// Return ownership to backend
	}
	if (d.trip)
		add_dive_to_trip(d.dive.get(), d.trip);
	dive *res = d.dive.release();			// Give up ownership of dive
	add_single_dive(d.idx, res);			// Return ownership to backend
	return res;
}

UndoAddDive::UndoAddDive(dive *d)
{
	setText(gettextFromC::tr("add dive"));
	// TODO: handle tags
	//saveTags();
	d->maxdepth.mm = 0;
	fixup_dive(d);
	diveToAdd.trip = d->divetrip;
	d->divetrip = nullptr;
	diveToAdd.idx = dive_get_insertion_index(d);
	d->number = get_dive_nr_at_idx(diveToAdd.idx);
	diveToAdd.dive.reset(clone_dive(d));
}

void UndoAddDive::redo()
{
	diveToRemove = addDive(diveToAdd);
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->dive_list()->unselectDives();
	MainWindow::instance()->dive_list()->selectDive(diveToAdd.idx, true);
	MainWindow::instance()->refreshDisplay();
}

void UndoAddDive::undo()
{
	// Simply remove the dive that was previously added
	diveToAdd = removeDive(diveToRemove);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

UndoDeleteDive::UndoDeleteDive(const QVector<struct dive*> &divesToDeleteIn) : divesToDelete(divesToDeleteIn)
{
	setText(tr("delete %n dive(s)", "", divesToDelete.size()));
}

void UndoDeleteDive::undo()
{
	for (auto it = divesToAdd.rbegin(); it != divesToAdd.rend(); ++it)
		divesToDelete.append(addDive(*it));

	mark_divelist_changed(true);
	divesToAdd.clear();

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoDeleteDive::redo()
{
	for (dive *d: divesToDelete)
		divesToAdd.push_back(removeDive(d));

	divesToDelete.clear();
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}


UndoShiftTime::UndoShiftTime(QVector<int> changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(tr("delete %n dive(s)", "", changedDives.size()));
}

void UndoShiftTime::undo()
{
	for (int i = 0; i < diveList.count(); i++) {
		dive *d = get_dive_by_uniq_id(diveList.at(i));
		d->when -= timeChanged;
	}
	// Changing times may have unsorted the dive table
	sort_table(&dive_table);
	mark_divelist_changed(true);

	// Negate the time-shift so that the next call does the reverse
	timeChanged = -timeChanged;

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoShiftTime::redo()
{
	// Same as undo(), since after undo() we reversed the timeOffset
	undo();
}


UndoRenumberDives::UndoRenumberDives(const QVector<QPair<int, int>> &divesToRenumberIn) : divesToRenumber(divesToRenumberIn)
{
	setText(tr("renumber %n dive(s)", "", divesToRenumber.count()));
}

void UndoRenumberDives::undo()
{
	for (auto &pair: divesToRenumber) {
		dive *d = get_dive_by_uniq_id(pair.first);
		if (!d)
			continue;
		std::swap(d->number, pair.second);
	}
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoRenumberDives::redo()
{
	// Redo and undo do the same thing!
	undo();
}

UndoRemoveDivesFromTrip::UndoRemoveDivesFromTrip(const QVector<dive *> &divesToRemoveIn) : divesToRemove(divesToRemoveIn)
{
	setText(tr("remove %n dive(s) from trip", "", divesToRemove.size()));
}

void UndoRemoveDivesFromTrip::undo()
{
	// first bring back the trip(s)
	for (auto &trip: tripsToAdd) {
		dive_trip *t = trip.release();	// Give up ownership
		insert_trip(&t);		// Return ownership to backend
	}
	tripsToAdd.clear();

	for (auto &pair: divesToAdd)
		add_dive_to_trip(pair.first, pair.second);
	divesToAdd.clear();

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoRemoveDivesFromTrip::redo()
{
	for (dive *d: divesToRemove) {
		// remove dive from trip - if this is the last dive in the trip
		// remove the whole trip.
		dive_trip *trip = unregister_dive_from_trip(d, false);
		if (!trip)
			continue;				// This was not part of a trip
		if (trip->nrdives == 0) {
			unregister_trip(trip);			// Remove trip from backend
			tripsToAdd.emplace_back(trip);		// Take ownership of trip
		}
		divesToAdd.emplace_back(d, trip);
	}
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}
