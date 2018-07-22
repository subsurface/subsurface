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

// This helper function calls removeDive() on a list of dives to be removed and
// returns a vector of corresponding DiveToAdd objects, which can later be readded.
// The passed in vector is cleared.
static std::vector<DiveToAdd> removeDives(std::vector<dive *> &divesToDelete)
{
	std::vector<DiveToAdd> res;
	res.reserve(divesToDelete.size());

	for (dive *d: divesToDelete)
		res.push_back(removeDive(d));
	divesToDelete.clear();

	return res;
}

// This helper function is the counterpart fo removeDives(): it calls addDive() on a list
// of dives to be (re)added and returns a vector of the added dives. It does this in reverse
// order, so that trips are created appropriately and indexing is correct.
// The passed in vector is cleared.
static std::vector<dive *> addDives(std::vector<DiveToAdd> &divesToAdd)
{
	std::vector<dive *> res;
	res.reserve(divesToAdd.size());

	for (auto it = divesToAdd.rbegin(); it != divesToAdd.rend(); ++it)
		res.push_back(addDive(*it));
	divesToAdd.clear();

	return res;
}

// This helper function renumbers dives according to an array of id/number pairs.
// The old numbers are stored in the array, thus calling this function twice has no effect.
// TODO: switch from uniq-id to indexes once all divelist-actions are controlled by "UndoCommands".
static void renumberDives(QVector<QPair<int, int>> &divesToRenumber)
{
	for (auto &pair: divesToRenumber) {
		dive *d = get_dive_by_uniq_id(pair.first);
		if (!d)
			continue;
		std::swap(d->number, pair.second);
	}
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

UndoDeleteDive::UndoDeleteDive(const QVector<struct dive*> &divesToDeleteIn) : divesToDelete(divesToDeleteIn.toStdVector())
{
	setText(tr("delete %n dive(s)", "", divesToDelete.size()));
}

void UndoDeleteDive::undo()
{
	divesToDelete = addDives(divesToAdd);

	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoDeleteDive::redo()
{
	divesToAdd = removeDives(divesToDelete);
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}


UndoShiftTime::UndoShiftTime(const QVector<dive *> &changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(tr("delete %n dive(s)", "", changedDives.size()));
}

void UndoShiftTime::undo()
{
	for (dive *d: diveList)
		d->when -= timeChanged;

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
	renumberDives(divesToRenumber);
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
	mark_divelist_changed(true);

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

UndoSplitDives::UndoSplitDives(dive *d, duration_t time)
{
	setText(gettextFromC::tr("split dive"));

	// Split the dive
	dive *new1, *new2;
	int idx = time.seconds < 0 ?
		split_dive_dont_insert(d, &new1, &new2) :
		split_dive_at_time_dont_insert(d, time, &new1, &new2);

	// If this didn't work, reset pointers so that redo() and undo() do nothing
	if (idx < 0) {
		diveToSplit = nullptr;
		divesToUnsplit[0] = divesToUnsplit[1];
		return;
	}

	diveToSplit = d;
	splitDives[0].dive.reset(new1);
	splitDives[0].trip = d->divetrip;
	splitDives[0].idx = idx;
	splitDives[1].dive.reset(new2);
	splitDives[1].trip = d->divetrip;
	splitDives[1].idx = idx + 1;
}

void UndoSplitDives::redo()
{
	if (!diveToSplit)
		return;
	divesToUnsplit[0] = addDive(splitDives[0]);
	divesToUnsplit[1] = addDive(splitDives[1]);
	unsplitDive = removeDive(diveToSplit);
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();
}

void UndoSplitDives::undo()
{
	if (!unsplitDive.dive)
		return;
	// Note: reverse order with respect to redo()
	diveToSplit = addDive(unsplitDive);
	splitDives[1] = removeDive(divesToUnsplit[1]);
	splitDives[0] = removeDive(divesToUnsplit[0]);
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();
}

UndoMergeDives::UndoMergeDives(const QVector <dive *> &dives)
{
	setText(gettextFromC::tr("merge dive"));

	// We start in redo mode
	diveToUnmerge = nullptr;

	// Just a safety check - if there's not two or more dives - do nothing
	// The caller should have made sure that this doesn't happen.
	if (dives.count() < 2) {
		qWarning() << "Merging less than two dives";
		return;
	}

	dive_trip *preferred_trip;
	OwningDivePtr d(merge_dives(dives[0], dives[1], dives[1]->when - dives[0]->when, false, &preferred_trip));

	// Set the preferred dive trip, so that for subsequent merges the better trip can be selected
	d->divetrip = preferred_trip;
	for (int i = 2; i < dives.count(); ++i) {
		d.reset(merge_dives(d.get(), dives[i], dives[i]->when - d->when, false, &preferred_trip));
		// Set the preferred dive trip, so that for subsequent merges the better trip can be selected
		d->divetrip = preferred_trip;
	}

	// We got our preferred trip, so now the reference can be deleted from the newly generated dive
	d->divetrip = nullptr;

	// The merged dive gets the number of the first dive
	d->number = dives[0]->number;

	// We will only renumber the remaining dives if the joined dives are consecutive.
	// Otherwise all bets are off concerning what the user wanted and doing nothing seems
	// like the best option.
	int idx = get_divenr(dives[0]);
	int num = dives.count();
	if (idx < 0 || idx + num > dive_table.nr) {
		// It was the callers responsibility to pass only known dives.
		// Something is seriously wrong - give up.
		qWarning() << "Merging unknown dives";
		return;
	}
	// std::equal compares two ranges. The parameters are (begin_range1, end_range1, begin_range2).
	// Here, we can compare C-arrays, because QVector guarantees contiguous storage.
	if (std::equal(&dives[0], &dives[0] + num, &dive_table.dives[idx]) &&
	    dives[0]->number && dives.last()->number && dives[0]->number < dives.last()->number) {
		// We have a consecutive set of dives. Rename all following dives according to the
		// number of erased dives. This considers that there might be missing numbers.
		// Comment copied from core/divelist.c:
		// So if you had a dive list  1 3 6 7 8, and you
		// merge 1 and 3, the resulting numbered list will
		// be 1 4 5 6, because we assume that there were
		// some missing dives (originally dives 4 and 5),
		// that now will still be missing (dives 2 and 3
		// in the renumbered world).
		//
		// Obviously the normal case is that everything is
		// consecutive, and the difference will be 1, so the
		// above example is not supposed to be normal.
		int diff = dives.last()->number - dives[0]->number;
		divesToRenumber.reserve(dive_table.nr - idx - num);
		int previousnr = dives[0]->number;
		for (int i = idx + num; i < dive_table.nr; ++i) {
			int newnr = dive_table.dives[i]->number - diff;

			// Stop renumbering if stuff isn't in order (see also core/divelist.c)
			if (newnr <= previousnr)
				break;
			divesToRenumber.append(QPair<int,int>(dive_table.dives[i]->id, newnr));
			previousnr = newnr;
		}
	}

	mergedDive.dive = std::move(d);
	mergedDive.idx = get_divenr(dives[0]);
	mergedDive.trip = preferred_trip;
	divesToMerge = dives.toStdVector();
}

void UndoMergeDives::redo()
{
	renumberDives(divesToRenumber);
	diveToUnmerge = addDive(mergedDive);
	unmergedDives = removeDives(divesToMerge);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();
}

void UndoMergeDives::undo()
{
	divesToMerge = addDives(unmergedDives);
	mergedDive = removeDive(diveToUnmerge);
	renumberDives(divesToRenumber);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
	MainWindow::instance()->refreshProfile();
}
