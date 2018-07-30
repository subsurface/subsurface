// SPDX-License-Identifier: GPL-2.0

#include "command_divelist.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/divelist.h"
#include "core/subsurface-qt/DiveListNotifier.h"

namespace Command {

// Generally, signals are sent in batches per trip. To avoid writing the same loop
// again and again, this template takes a vector of trip / dive pairs, sorts it
// by trip and then calls a function-object with trip and a QVector of dives in that trip.
// Input parameters:
//	- dives: a vector of trip,dive pairs, which will be sorted and processed in batches by trip.
//	- action: a function object, taking a trip-pointer and a QVector of dives, which will be called for each batch.
template<typename Function>
void processByTrip(std::vector<std::pair<dive_trip *, dive *>> &dives, Function action)
{
	// Use std::tie for lexicographical sorting of trip, then start-time
	std::sort(dives.begin(), dives.end(),
		  [](const std::pair<dive_trip *, dive *> &e1, const std::pair<dive_trip *, dive *> &e2)
		  { return std::tie(e1.first, e1.second->when) < std::tie(e2.first, e2.second->when); });

	// Then, process the dives in batches by trip
	size_t i, j; // Begin and end of batch
	for (i = 0; i < dives.size(); i = j) {
		dive_trip *trip = dives[i].first;
		for (j = i + 1; j < dives.size() && dives[j].first == trip; ++j)
			; // pass
		// Copy dives into a QVector. Some sort of "range_view" would be ideal, but Qt doesn't work this way.
		QVector<dive *> divesInTrip(j - i);
		for (size_t k = i; k < j; ++k)
			divesInTrip[k - i] = dives[k].second;

		// Finally, emit the signal
		action(trip, divesInTrip);
	}
}


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
	if (d.tripToAdd)
		insert_trip_dont_merge(d.tripToAdd.release()); // Return ownership to backend
	if (d.trip)
		add_dive_to_trip(d.dive.get(), d.trip);
	dive *res = d.dive.release();		// Give up ownership of dive
	add_single_dive(d.idx, res);		// Return ownership to backend

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

	// We send one dives-deleted signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in a array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(res.size());
	for (const DiveToAdd &entry: res)
		dives.push_back({ entry.trip, entry.dive.get() });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Now, let's check if this trip is supposed to be deleted, by checking if it was marked
		// as "add it". We could be smarter here, but let's just check the whole array for brevity.
		bool deleteTrip = trip &&
				  std::find_if(res.begin(), res.end(), [trip](const DiveToAdd &entry)
					       { return entry.tripToAdd.get() == trip; }) != res.end();
		emit diveListNotifier.divesDeleted(trip, deleteTrip, divesInTrip);
	});
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

	// At the end of the function, to send the proper dives-added signals,
	// we the the list of added trips. Create this list now.
	std::vector<dive_trip *> addedTrips;
	for (const DiveToAdd &entry: divesToAdd) {
		if (entry.tripToAdd)
			addedTrips.push_back(entry.tripToAdd.get());
	}

	// Now, add the dives
	for (auto it = divesToAdd.rbegin(); it != divesToAdd.rend(); ++it)
		res.push_back(addDive(*it));
	divesToAdd.clear();

	// We send one dives-deleted signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in a array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(res.size());
	for (dive *d: res)
		dives.push_back({ d->divetrip, d });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Now, let's check if this trip is supposed to be created, by checking if it was marked
		// as "add it". We could be smarter here, but let's just check the whole array for brevity.
		bool createTrip = trip && std::find(addedTrips.begin(), addedTrips.end(), trip) != addedTrips.end();
		// Finally, emit the signal
		emit diveListNotifier.divesAdded(trip, createTrip, divesInTrip);
	});
	return res;
}

// This helper function renumbers dives according to an array of id/number pairs.
// The old numbers are stored in the array, thus calling this function twice has no effect.
// TODO: switch from uniq-id to indexes once all divelist-actions are controlled by undo-able commands
static void renumberDives(QVector<QPair<int, int>> &divesToRenumber)
{
	for (auto &pair: divesToRenumber) {
		dive *d = get_dive_by_uniq_id(pair.first);
		if (!d)
			continue;
		std::swap(d->number, pair.second);
	}

	// Emit changed signals per trip.
	// First, collect all dives and sort by trip
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(divesToRenumber.size());
	for (const auto &pair: divesToRenumber) {
		dive *d = get_dive_by_uniq_id(pair.first);
		dives.push_back({ d->divetrip, d });
	}

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		emit diveListNotifier.divesChanged(trip, divesInTrip);
	});
}

// This helper function moves a dive to a trip. The old trip is recorded in the
// passed-in structure. This means that calling the function twice on the same
// object is a no-op concerning the dive. If the old trip was deleted from the
// core, an owning pointer to the removed trip is returned, otherwise a null pointer.
static OwningTripPtr moveDiveToTrip(DiveToTrip &diveToTrip)
{
	// Firstly, check if we move to the same trip and bail if this is a no-op.
	if (diveToTrip.trip == diveToTrip.dive->divetrip)
		return {};

	// Remove from old trip
	OwningTripPtr res;

	// Remove dive from trip - if this is the last dive in the trip, remove the whole trip.
	dive_trip *trip = unregister_dive_from_trip(diveToTrip.dive, false);
	if (trip && trip->nrdives == 0) {
		unregister_trip(trip);		// Remove trip from backend
		res.reset(trip);
	}

	// Store old trip and get new trip we should associate this dive with
	std::swap(trip, diveToTrip.trip);
	add_dive_to_trip(diveToTrip.dive, trip);
	return res;
}

// This helper function moves a set of dives between trips using the
// moveDiveToTrip function. Before doing so, it adds the necessary trips to
// the core. Trips that are removed from the core because they are empty
// are recorded in the passed in struct. The vectors of trips and dives
// are reversed. Thus, calling the function twice on the same object is
// a no-op.
static void moveDivesBetweenTrips(DivesToTrip &dives)
{
	// We collect an array of created trips so that we can instruct
	// the model to create a new entry
	std::vector<dive_trip *> createdTrips;
	createdTrips.reserve(dives.tripsToAdd.size());

	// First, bring back the trip(s)
	for (OwningTripPtr &trip: dives.tripsToAdd) {
		dive_trip *t = trip.release();	// Give up ownership
		createdTrips.push_back(t);
		insert_trip_dont_merge(t);	// Return ownership to backend
	}
	dives.tripsToAdd.clear();

	for (DiveToTrip &dive: dives.divesToMove) {
		OwningTripPtr tripToAdd = moveDiveToTrip(dive);
		// register trips that we'll have to readd
		if (tripToAdd)
			dives.tripsToAdd.push_back(std::move(tripToAdd));
	}

	// We send one signal per from-trip/to-trip pair.
	// First, collect all dives in a struct and sort by from-trip/to-trip.
	struct DiveMoved {
		dive_trip *from;
		dive_trip *to;
		dive *d;
	};
	std::vector<DiveMoved> divesMoved;
	divesMoved.reserve(dives.divesToMove.size());
	for (const DiveToTrip &entry: dives.divesToMove)
		divesMoved.push_back({ entry.trip, entry.dive->divetrip, entry.dive });

	// Sort lexicographically by from-trip, to-trip and by start-time.
	// Use std::tie() for lexicographical sorting.
	std::sort(divesMoved.begin(), divesMoved.end(), [] ( const DiveMoved &d1, const DiveMoved &d2)
		  { return std::tie(d1.from, d1.to, d1.d->when) < std::tie(d2.from, d2.to, d2.d->when); });

	// Now, process the dives in batches by trip
	// TODO: this is a bit different from the cases above, so we don't use the processByTrip template,
	// but repeat the loop here. We might think about generalizing the template, if more of such
	// "special cases" appear.
	size_t i, j; // Begin and end of batch
	for (i = 0; i < divesMoved.size(); i = j) {
		dive_trip *from = divesMoved[i].from;
		dive_trip *to = divesMoved[i].to;
		for (j = i + 1; j < divesMoved.size() && divesMoved[j].from == from && divesMoved[j].to == to; ++j)
			; // pass
		// Copy dives into a QVector. Some sort of "range_view" would be ideal, but Qt doesn't work this way.
		QVector<dive *> divesInTrip(j - i);
		for (size_t k = i; k < j; ++k)
			divesInTrip[k - i] = divesMoved[k].d;

		// Check if the from-trip was deleted: If yes, it was recorded in the tripsToAdd structure
		bool deleteFrom = from &&
				  std::find_if(dives.tripsToAdd.begin(), dives.tripsToAdd.end(),
					       [from](const OwningTripPtr &trip) { return trip.get() == from; }) != dives.tripsToAdd.end();
		// Check if the to-trip has to be created. For this purpose, we saved an array of trips to be created.
		bool createTo = false;
		if (to) {
			// Check if the element is there...
			auto it = std::find(createdTrips.begin(), createdTrips.end(), to);

			// ...if it is - remove it as we don't want the model to create the trip twice!
			if (it != createdTrips.end()) {
				createTo = true;
				// erase/remove would be more performant, but this is irrelevant in the big scheme of things.
				createdTrips.erase(it);
			}
		}

		// Finally, emit the signal
		emit diveListNotifier.divesMovedBetweenTrips(from, to, deleteFrom, createTo, divesInTrip);
	}

	// Reverse the tripsToAdd and the divesToAdd, so that on undo/redo the operations
	// will be performed in reverse order.
	std::reverse(dives.tripsToAdd.begin(), dives.tripsToAdd.end());
	std::reverse(dives.divesToMove.begin(), dives.divesToMove.end());
}

AddDive::AddDive(dive *d, bool autogroup)
{
	setText(tr("add dive"));
	d->maxdepth.mm = 0;
	fixup_dive(d);
	d->divetrip = nullptr;

	// Get an owning pointer to a copy of the dive
	// Note: this destroys the old dive!
	OwningDivePtr divePtr(clone_dive(d));

	// If we alloc a new-trip for autogrouping, get an owning pointer to it.
	OwningTripPtr allocTrip;
	dive_trip *trip = nullptr;
	if (autogroup) {
		bool alloc;
		trip = get_trip_for_new_dive(divePtr.get(), &alloc);
		if (alloc)
			allocTrip.reset(trip);
	}

	int idx = dive_get_insertion_index(divePtr.get());
	divePtr->number = get_dive_nr_at_idx(idx);

	divesToAdd.push_back({ std::move(divePtr), std::move(allocTrip), trip, idx });
}

bool AddDive::workToBeDone()
{
	return true;
}

void AddDive::redo()
{
	int idx = divesToAdd[0].idx;
	divesToRemove = addDives(divesToAdd);
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->dive_list()->unselectDives();
	MainWindow::instance()->dive_list()->selectDive(idx, true);

	// Exit from edit mode, but don't recalculate dive list
	// TODO: Remove edit mode
	MainWindow::instance()->refreshDisplay(false);
}

void AddDive::undo()
{
	// Simply remove the dive that was previously added
	divesToAdd = removeDives(divesToRemove);

	// Exit from edit mode, but don't recalculate dive list
	// TODO: Remove edit mode
	MainWindow::instance()->refreshDisplay(false);
}

DeleteDive::DeleteDive(const QVector<struct dive*> &divesToDeleteIn) : divesToDelete(divesToDeleteIn.toStdVector())
{
	setText(tr("delete %n dive(s)", "", divesToDelete.size()));
}

bool DeleteDive::workToBeDone()
{
	return !divesToDelete.empty();
}

void DeleteDive::undo()
{
	divesToDelete = addDives(divesToAdd);
	mark_divelist_changed(true);
}

void DeleteDive::redo()
{
	divesToAdd = removeDives(divesToDelete);
	mark_divelist_changed(true);
}


ShiftTime::ShiftTime(const QVector<dive *> &changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(tr("shift time of %n dives", "", changedDives.count()));
}

void ShiftTime::redo()
{
	for (dive *d: diveList)
		d->when -= timeChanged;

	// Changing times may have unsorted the dive table
	sort_table(&dive_table);

	// We send one dives-deleted signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in a array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(diveList.size());
	for (dive *d: diveList)
		dives.push_back({ d->divetrip, d });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		emit diveListNotifier.divesTimeChanged(trip, timeChanged, divesInTrip);
	});

	// Negate the time-shift so that the next call does the reverse
	timeChanged = -timeChanged;

	mark_divelist_changed(true);
}

bool ShiftTime::workToBeDone()
{
	return !diveList.isEmpty();
}

void ShiftTime::undo()
{
	// Same as redo(), since after redo() we reversed the timeOffset
	redo();
}


RenumberDives::RenumberDives(const QVector<QPair<int, int>> &divesToRenumberIn) : divesToRenumber(divesToRenumberIn)
{
	setText(tr("renumber %n dive(s)", "", divesToRenumber.count()));
}

void RenumberDives::undo()
{
	renumberDives(divesToRenumber);
	mark_divelist_changed(true);
}

bool RenumberDives::workToBeDone()
{
	return !divesToRenumber.isEmpty();
}

void RenumberDives::redo()
{
	// Redo and undo do the same thing!
	undo();
}

bool TripBase::workToBeDone()
{
	return !divesToMove.divesToMove.empty();
}

void TripBase::redo()
{
	moveDivesBetweenTrips(divesToMove);

	mark_divelist_changed(true);
}

void TripBase::undo()
{
	// Redo and undo do the same thing!
	redo();
}

RemoveDivesFromTrip::RemoveDivesFromTrip(const QVector<dive *> &divesToRemove)
{
	setText(tr("remove %n dive(s) from trip", "", divesToRemove.size()));
	divesToMove.divesToMove.reserve(divesToRemove.size());
	for (dive *d: divesToRemove)
		divesToMove.divesToMove.push_back( {d, nullptr} );
}

RemoveAutogenTrips::RemoveAutogenTrips()
{
	setText(tr("remove autogenerated trips"));
	// TODO: don't touch core-innards directly
	int i;
	struct dive *dive;
	for_each_dive(i, dive) {
		if (dive->divetrip && dive->divetrip->autogen)
			divesToMove.divesToMove.push_back( {dive, nullptr} );
	}
}

AddDivesToTrip::AddDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip)
{
	setText(tr("add %n dives to trip", "", divesToAddIn.size()));
	for (dive *d: divesToAddIn)
		divesToMove.divesToMove.push_back( {d, trip} );
}

CreateTrip::CreateTrip(const QVector<dive *> &divesToAddIn)
{
	setText(tr("create trip"));

	if (divesToAddIn.isEmpty())
		return;

	dive_trip *trip = create_trip_from_dive(divesToAddIn[0]);
	divesToMove.tripsToAdd.emplace_back(trip);
	for (dive *d: divesToAddIn)
		divesToMove.divesToMove.push_back( {d, trip} );
}

AutogroupDives::AutogroupDives()
{
	setText(tr("autogroup dives"));

	dive_trip *trip;
	bool alloc;
	int from, to;
	for(int i = 0; (trip = get_dives_to_autogroup(i, &from, &to, &alloc)) != NULL; i = to) {
		// If this is an allocated trip, take ownership
		if (alloc)
			divesToMove.tripsToAdd.emplace_back(trip);
		for (int j = from; j < to; ++j)
			divesToMove.divesToMove.push_back( { get_dive(j), trip } );
	}
}

MergeTrips::MergeTrips(dive_trip *trip1, dive_trip *trip2)
{
	if (trip1 == trip2)
		return;
	dive_trip *newTrip = combine_trips_create(trip1, trip2);
	divesToMove.tripsToAdd.emplace_back(newTrip);
	for (dive *d = trip1->dives; d; d = d->next)
		divesToMove.divesToMove.push_back( { d, newTrip } );
	for (dive *d = trip2->dives; d; d = d->next)
		divesToMove.divesToMove.push_back( { d, newTrip } );
}

SplitDives::SplitDives(dive *d, duration_t time)
{
	setText(tr("split dive"));

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

bool SplitDives::workToBeDone()
{
	return !!diveToSplit;
}

void SplitDives::redo()
{
	if (!diveToSplit)
		return;
	divesToUnsplit[0] = addDive(splitDives[0]);
	divesToUnsplit[1] = addDive(splitDives[1]);
	unsplitDive = removeDive(diveToSplit);
	mark_divelist_changed(true);
}

void SplitDives::undo()
{
	if (!unsplitDive.dive)
		return;
	// Note: reverse order with respect to redo()
	diveToSplit = addDive(unsplitDive);
	splitDives[1] = removeDive(divesToUnsplit[1]);
	splitDives[0] = removeDive(divesToUnsplit[0]);
	mark_divelist_changed(true);
}

MergeDives::MergeDives(const QVector <dive *> &dives)
{
	setText(tr("merge dive"));

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

bool MergeDives::workToBeDone()
{
	return !!mergedDive.dive;
}

void MergeDives::redo()
{
	renumberDives(divesToRenumber);
	diveToUnmerge = addDive(mergedDive);
	unmergedDives = removeDives(divesToMerge);
}

void MergeDives::undo()
{
	divesToMerge = addDives(unmergedDives);
	mergedDive = removeDive(diveToUnmerge);
	renumberDives(divesToRenumber);
}

} // namespace Command
