// SPDX-License-Identifier: GPL-2.0

#include "command_divelist.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/divelist.h"
#include "core/display.h" // for amount_selected
#include "core/qthelper.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/filtermodels.h"

#include <array>

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
// If the trip the dive belongs to becomes empty, it is removed and added to the tripsToAdd vector.
// It is crucial that dives are added in reverse order of deletion, so that the indices are correctly
// set and that the trips are added before they are used!
DiveToAdd DiveListBase::removeDive(struct dive *d, std::vector<OwningTripPtr> &tripsToAdd)
{
	// If the dive to be removed is selected, we will inform the frontend
	// later via a signal that the dive changed.
	if (d->selected)
		selectionChanged = true;

	// If the dive was the current dive, reset the current dive. The calling
	// command is responsible of finding a new dive.
	if (d == current_dive) {
		selectionChanged = true; // Should have been set above, as current dive is always selected.
		current_dive = nullptr;
	}

	DiveToAdd res;
	res.idx = get_divenr(d);
	if (res.idx < 0)
		qWarning() << "Deletion of unknown dive!";

	// remove dive from trip and site - if this is the last dive in the trip
	// remove the whole trip.
	res.trip = unregister_dive_from_trip(d);
	res.site = unregister_dive_from_dive_site(d);
	if (res.trip && res.trip->dives.nr == 0) {
		unregister_trip(res.trip, &trip_table);	// Remove trip from backend
		tripsToAdd.emplace_back(res.trip);	// Take ownership of trip
	}

	res.dive.reset(unregister_dive(res.idx));	// Remove dive from backend

	return res;
}

// This helper function adds a dive and returns ownership to the backend. It may also add a dive trip.
// It is crucial that dives are added in reverse order of deletion (see comment above)!
// Returns pointer to added dive (which is owned by the backend!)
dive *DiveListBase::addDive(DiveToAdd &d)
{
	if (d.trip)
		add_dive_to_trip(d.dive.get(), d.trip);
	if (d.site)
		add_dive_to_dive_site(d.dive.get(), d.site);
	dive *res = d.dive.release();		// Give up ownership of dive

	// Set the filter flag according to current filter settings
	bool show = MultiFilterSortModel::instance()->showDive(res);
	res->hidden_by_filter = !show;

	add_single_dive(d.idx, res);		// Return ownership to backend
	invalidate_dive_cache(res);		// Ensure that dive is written in git_save()

	// If the dive to be removed is selected, we will inform the frontend
	// later via a signal that the dive changed.
	if (res->selected)
		selectionChanged = true;

	return res;
}

// This helper function calls removeDive() on a list of dives to be removed and
// returns a vector of corresponding DiveToAdd objects, which can later be readded.
// Moreover, a vector of deleted trips is returned, if trips became empty.
// The passed in vector is cleared.
DivesAndTripsToAdd DiveListBase::removeDives(DivesAndSitesToRemove &divesAndSitesToDelete)
{
	std::vector<DiveToAdd> divesToAdd;
	std::vector<OwningTripPtr> tripsToAdd;
	std::vector<OwningDiveSitePtr> sitesToAdd;
	divesToAdd.reserve(divesAndSitesToDelete.dives.size());
	sitesToAdd.reserve(divesAndSitesToDelete.sites.size());

	for (dive *d: divesAndSitesToDelete.dives)
		divesToAdd.push_back(removeDive(d, tripsToAdd));
	divesAndSitesToDelete.dives.clear();

	for (dive_site *ds: divesAndSitesToDelete.sites) {
		unregister_dive_site(ds);
		sitesToAdd.emplace_back(ds);
	}
	divesAndSitesToDelete.sites.clear();

	// We send one dives-deleted signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in an array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(divesToAdd.size());
	for (const DiveToAdd &entry: divesToAdd)
		dives.push_back({ entry.trip, entry.dive.get() });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Check if this trip is supposed to be deleted, by checking if it was marked as "add it".
		bool deleteTrip = trip &&
				  std::find_if(tripsToAdd.begin(), tripsToAdd.end(), [trip](const OwningTripPtr &ptr)
					       { return ptr.get() == trip; }) != tripsToAdd.end();
		emit diveListNotifier.divesDeleted(trip, deleteTrip, divesInTrip);
	});
	return { std::move(divesToAdd), std::move(tripsToAdd), std::move(sitesToAdd) };
}

// This helper function is the counterpart fo removeDives(): it calls addDive() on a list
// of dives to be (re)added and returns a vector of the added dives. It does this in reverse
// order, so that trips are created appropriately and indexing is correct.
// The passed in vector is cleared.
DivesAndSitesToRemove DiveListBase::addDives(DivesAndTripsToAdd &toAdd)
{
	std::vector<dive *> res;
	std::vector<dive_site *> sites;
	res.resize(toAdd.dives.size());
	sites.reserve(toAdd.sites.size());

	// Now, add the dives
	// Note: the idiomatic STL-way would be std::transform, but let's use a loop since
	// that is closer to classical C-style.
	auto it2 = res.rbegin();
	for (auto it = toAdd.dives.rbegin(); it != toAdd.dives.rend(); ++it, ++it2)
		*it2 = addDive(*it);
	toAdd.dives.clear();

	// If the dives belong to new trips, add these as well.
	// Remember the pointers so that we can later check if a trip was newly added
	std::vector<dive_trip *> addedTrips;
	addedTrips.reserve(toAdd.trips.size());
	for (OwningTripPtr &trip: toAdd.trips) {
		addedTrips.push_back(trip.get());
		insert_trip(trip.release(), &trip_table); // Return ownership to backend
	}
	toAdd.trips.clear();

	// Finally, add any necessary dive sites
	for (OwningDiveSitePtr &ds: toAdd.sites) {
		sites.push_back(ds.get());
		register_dive_site(ds.release()); // Return ownership to backend
	}
	toAdd.sites.clear();

	// We send one dives-deleted signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in a array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(res.size());
	for (dive *d: res)
		dives.push_back({ d->divetrip, d });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Now, let's check if this trip is supposed to be created, by checking if it was marked as "add it".
		bool createTrip = trip && std::find(addedTrips.begin(), addedTrips.end(), trip) != addedTrips.end();
		// Finally, emit the signal
		emit diveListNotifier.divesAdded(trip, createTrip, divesInTrip);
	});
	return { res, sites };
}

// This helper function renumbers dives according to an array of id/number pairs.
// The old numbers are stored in the array, thus calling this function twice has no effect.
// TODO: switch from uniq-id to indices once all divelist-actions are controlled by undo-able commands
static void renumberDives(QVector<QPair<dive *, int>> &divesToRenumber)
{
	for (auto &pair: divesToRenumber) {
		dive *d = pair.first;
		if (!d)
			continue;
		std::swap(d->number, pair.second);
		invalidate_dive_cache(d);
	}

	// Emit changed signals per trip.
	// First, collect all dives and sort by trip
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(divesToRenumber.size());
	for (const auto &pair: divesToRenumber) {
		dive *d = pair.first;
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
	dive_trip *trip = unregister_dive_from_trip(diveToTrip.dive);
	if (trip && trip->dives.nr == 0) {
		unregister_trip(trip, &trip_table);	// Remove trip from backend
		res.reset(trip);
	}

	// Store old trip and get new trip we should associate this dive with
	std::swap(trip, diveToTrip.trip);
	add_dive_to_trip(diveToTrip.dive, trip);
	invalidate_dive_cache(diveToTrip.dive);		// Ensure that dive is written in git_save()
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
		insert_trip(t, &trip_table);	// Return ownership to backend
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

		// Check if the from-trip was deleted: If yes, it was recorded in the tripsToAdd structure.
		// Only set the flag if this is that last time this trip is featured.
		bool deleteFrom = from &&
				  std::find_if(divesMoved.begin() + j, divesMoved.end(), // Is this the last occurence of "from"?
					       [from](const DiveMoved &entry) { return entry.from == from; }) == divesMoved.end() &&
				  std::find_if(dives.tripsToAdd.begin(), dives.tripsToAdd.end(), // Is "from" in tripsToAdd?
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

// Turn current selection into a vector.
// TODO: This could be made much more efficient if we kept a sorted list of selected dives!
static std::vector<dive *> getDiveSelection()
{
	std::vector<dive *> res;
	res.reserve(amount_selected);

	int i;
	dive *d;
	for_each_dive(i, d) {
		if (d->selected)
			res.push_back(d);
	}
	return res;
}

void DiveListBase::initWork()
{
	selectionChanged = false;
}

void DiveListBase::finishWork()
{
	if (selectionChanged) // If the selection changed -> tell the frontend
		emit diveListNotifier.selectionChanged();
}

// Set the current dive either from a list of selected dives,
// or a newly selected dive. In both cases, try to select the
// dive that is newer that is newer than the given date.
// This mimics the old behavior when the current dive changed.
static void setClosestCurrentDive(timestamp_t when, const std::vector<dive *> &selection)
{
	// Start from back until we get the first dive that is before
	// the supposed-to-be selected dive. (Note: this mimics the
	// old behavior when the current dive changed).
	for (auto it = selection.rbegin(); it < selection.rend(); ++it) {
		if ((*it)->when > when && !(*it)->hidden_by_filter) {
			current_dive = *it;
			return;
		}
	}

	// We didn't find a more recent selected dive -> try to
	// find *any* visible selected dive.
	for (dive *d: selection) {
		if (!d->hidden_by_filter) {
			current_dive = d;
			return;
		}
	}

	// No selected dive is visible! Take the closest dive. Note, this might
	// return null, but that just means unsetting the current dive (as no
	// dive is visible anyway).
	current_dive = find_next_visible_dive(when);
}

// Rese the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "seletion" is empty).
void DiveListBase::restoreSelection(const std::vector<dive *> &selection, dive *currentDive)
{
	// To do so, generate vectors of dives to be selected and deselected.
	// We send signals batched by trip, so keep track of trip/dive pairs.
	std::vector<std::pair<dive_trip *, dive *>> divesToSelect;
	std::vector<std::pair<dive_trip *, dive *>> divesToDeselect;

	// TODO: We might want to keep track of selected dives in a more efficient way!
	int i;
	dive *d;
	amount_selected = 0; // We recalculate amount_selected
	for_each_dive(i, d) {
		// We only modify dives that are currently visible.
		if (d->hidden_by_filter) {
			d->selected = false; // Note, not necessary, just to be sure
					     // that we get amount_selected right
			continue;
		}

		// Search the dive in the list of selected dives.
		// TODO: By sorting the list in the same way as the backend, this could be made more efficient.
		bool newState = std::find(selection.begin(), selection.end(), d) != selection.end();

		// TODO: Instead of using select_dive() and deselect_dive(), we set selected directly.
		// The reason is that deselect() automatically sets a new current dive, which we
		// don't want, as we set it later anyway.
		// There is other parts of the C++ code that touches the innards directly, but
		// ultimately this should be pushed down to C.
		if (newState && !d->selected) {
			d->selected = true;
			++amount_selected;
			divesToSelect.push_back({ d->divetrip, d });
		} else if (!newState && d->selected) {
			d->selected = false;
			divesToDeselect.push_back({ d->divetrip, d });
		}
	}

	// Send the select and deselect signals
	processByTrip(divesToSelect, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		emit diveListNotifier.divesSelected(trip, divesInTrip);
	});
	processByTrip(divesToDeselect, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		emit diveListNotifier.divesDeselected(trip, divesInTrip);
	});

	bool currentDiveChanged = false;
	// If currentDive is null, we have no current dive. In such a case always
	// signal the frontend.
	if (!currentDive) {
		currentDiveChanged = true;
		emit diveListNotifier.currentDiveChanged();
	} else if (current_dive != currentDive) {
		currentDiveChanged = true;

		// We cannot simply change the currentd dive to the given dive.
		// It might be hidden by a filter and thus not be selected.
		if (currentDive->selected)
			// Current dive is visible and selected. Excellent.
			current_dive = currentDive;
		else
			// Current not visible -> find a different dive.
			setClosestCurrentDive(currentDive->when, selection);
		emit diveListNotifier.currentDiveChanged();
	}

	// If anything changed (selection or current dive), send a final signal.
	if (!divesToSelect.empty() || !divesToDeselect.empty() || currentDiveChanged)
		selectionChanged = true;
}

void DiveListBase::undo()
{
	auto marker = diveListNotifier.enterCommand();
	initWork();
	undoit();
	finishWork();
}

void DiveListBase::redo()
{
	auto marker = diveListNotifier.enterCommand();
	initWork();
	redoit();
	finishWork();
}

AddDive::AddDive(dive *d, const QString &newDS, bool autogroup, bool newNumber)
{
	setText(tr("add dive"));
	// By convention, d is "displayed dive" and can be overwritten.
	d->maxdepth.mm = 0;
	d->dc.maxdepth.mm = 0;
	fixup_dive(d);

	// Create new dive site if requested.
	if (!newDS.isEmpty()) {
		struct dive_site *ds = alloc_dive_site();
		ds->name = copy_qstring(newDS);
		d->dive_site = ds;
		divesToAdd.sites.emplace_back(ds);
	}

	// Get an owning pointer to a copied or moved dive
	// Note: if move is true, this destroys the old dive!
	OwningDivePtr divePtr(clone_dive(d));
	divePtr->selected = false; // If we clone a planned dive, it might have been selected.
				   // We have to clear the flag, as selections will be managed
				   // on dive-addition.

	// If we alloc a new-trip for autogrouping, get an owning pointer to it.
	OwningTripPtr allocTrip;
	dive_trip *trip = divePtr->divetrip;
	dive_site *site = divePtr->dive_site;
	// We have to delete the pointers to trip and site, because this would prevent the core from adding to the
	// trip or site and we would get the count-of-dives in the trip or site wrong. Yes, that's all horribly subtle!
	divePtr->divetrip = nullptr;
	divePtr->dive_site = nullptr;
	if (!trip && autogroup) {
		bool alloc;
		trip = get_trip_for_new_dive(divePtr.get(), &alloc);
		if (alloc)
			allocTrip.reset(trip);
	}

	int idx = dive_table_get_insertion_index(&dive_table, divePtr.get());
	if (newNumber)
		divePtr->number = get_dive_nr_at_idx(idx);

	divesToAdd.dives.push_back({ std::move(divePtr), trip, site, idx });
	if (allocTrip)
		divesToAdd.trips.push_back(std::move(allocTrip));
}

bool AddDive::workToBeDone()
{
	return true;
}

void AddDive::redoit()
{
	// Remember selection so that we can undo it
	selection = getDiveSelection();
	currentDive = current_dive;

	divesAndSitesToRemove = addDives(divesToAdd);
	sort_trip_table(&trip_table); // Though unlikely, adding a dive may reorder trips
	mark_divelist_changed(true);

	// Select the newly added dive
	restoreSelection(divesAndSitesToRemove.dives, divesAndSitesToRemove.dives[0]);

	// Exit from edit mode, but don't recalculate dive list
	// TODO: Remove edit mode
	MainWindow::instance()->refreshDisplay(false);
}

void AddDive::undoit()
{
	// Simply remove the dive that was previously added...
	divesToAdd = removeDives(divesAndSitesToRemove);
	sort_trip_table(&trip_table); // Though unlikely, removing a dive may reorder trips

	// ...and restore the selection
	restoreSelection(selection, currentDive);

	// Exit from edit mode, but don't recalculate dive list
	// TODO: Remove edit mode
	MainWindow::instance()->refreshDisplay(false);
}

ImportDives::ImportDives(struct dive_table *dives, struct trip_table *trips, struct dive_site_table *sites, int flags, const QString &source)
{
	setText(tr("import %n dive(s) from %1", "", dives->nr).arg(source));

	struct dive_table dives_to_add = { 0 };
	struct dive_table dives_to_remove = { 0 };
	struct trip_table trips_to_add = { 0 };
	struct dive_site_table sites_to_add = { 0 };
	process_imported_dives(dives, trips, sites, flags, &dives_to_add, &dives_to_remove, &trips_to_add, &sites_to_add);

	// Add trips to the divesToAdd.trips structure
	divesToAdd.trips.reserve(trips_to_add.nr);
	for (int i = 0; i < trips_to_add.nr; ++i)
		divesToAdd.trips.emplace_back(trips_to_add.trips[i]);

	// Add sites to the divesToAdd.sites structure
	divesToAdd.sites.reserve(sites_to_add.nr);
	for (int i = 0; i < sites_to_add.nr; ++i)
		divesToAdd.sites.emplace_back(sites_to_add.dive_sites[i]);

	// Add dives to the divesToAdd.dives structure
	divesToAdd.dives.reserve(dives_to_add.nr);
	for (int i = 0; i < dives_to_add.nr; ++i) {
		OwningDivePtr divePtr(dives_to_add.dives[i]);
		divePtr->selected = false; // See above in AddDive::AddDive()
		dive_trip *trip = divePtr->divetrip;
		divePtr->divetrip = nullptr; // See above in AddDive::AddDive()
		dive_site *site = divePtr->dive_site;
		divePtr->dive_site = nullptr; // See above in AddDive::AddDive()
		int idx = dive_table_get_insertion_index(&dive_table, divePtr.get());

		// Note: The dives are added in reverse order of the divesToAdd array.
		// This, and the fact that we populate the array in chronological order
		// means that wo do *not* have to manipulated the indices.
		// Yes, that's all horribly subtle.
		divesToAdd.dives.push_back({ std::move(divePtr), trip, site, idx });
	}

	// Add dive to be deleted to the divesToRemove structure
	divesAndSitesToRemove.dives.reserve(dives_to_remove.nr);
	for (int i = 0; i < dives_to_remove.nr; ++i)
		divesAndSitesToRemove.dives.push_back(dives_to_remove.dives[i]);
}

bool ImportDives::workToBeDone()
{
	return !divesToAdd.dives.empty();
}

void ImportDives::redoit()
{
	// Remember selection so that we can undo it
	currentDive = current_dive;

	// Add new dives and sites
	DivesAndSitesToRemove divesAndSitesToRemoveNew = addDives(divesToAdd);

	// Remove old dives and sites
	divesToAdd = removeDives(divesAndSitesToRemove);

	// Select the newly added dives
	restoreSelection(divesAndSitesToRemoveNew.dives, divesAndSitesToRemoveNew.dives.back());

	// Remember dives and sites to remove
	divesAndSitesToRemove = std::move(divesAndSitesToRemoveNew);

	mark_divelist_changed(true);
}

void ImportDives::undoit()
{
	// Add new dives and sites
	DivesAndSitesToRemove divesAndSitesToRemoveNew = addDives(divesToAdd);

	// Remove old dives and sites
	divesToAdd = removeDives(divesAndSitesToRemove);

	// Remember dives and sites to remove
	divesAndSitesToRemove = std::move(divesAndSitesToRemoveNew);

	// ...and restore the selection
	restoreSelection(selection, currentDive);

	mark_divelist_changed(true);
}

DeleteDive::DeleteDive(const QVector<struct dive*> &divesToDeleteIn)
{
	divesToDelete.dives = divesToDeleteIn.toStdVector();
	setText(tr("delete %n dive(s)", "", divesToDelete.dives.size()));
}

bool DeleteDive::workToBeDone()
{
	return !divesToDelete.dives.empty();
}

void DeleteDive::undoit()
{
	divesToDelete = addDives(divesToAdd);
	sort_trip_table(&trip_table); // Though unlikely, removing a dive may reorder trips
	mark_divelist_changed(true);

	// Select all re-added dives and make the first one current
	dive *currentDive = !divesToDelete.dives.empty() ? divesToDelete.dives[0] : nullptr;
	restoreSelection(divesToDelete.dives, currentDive);
}

void DeleteDive::redoit()
{
	divesToAdd = removeDives(divesToDelete);
	sort_trip_table(&trip_table); // Though unlikely, adding a dive may reorder trips
	mark_divelist_changed(true);

	// Deselect all dives and select dive that was close to the first deleted dive
	dive *newCurrent = nullptr;
	if (!divesToAdd.dives.empty()) {
		timestamp_t when = divesToAdd.dives[0].dive->when;
		newCurrent = find_next_visible_dive(when);
	}
	if (newCurrent)
		restoreSelection(std::vector<dive *>{ newCurrent }, newCurrent);
	else
		restoreSelection(std::vector<dive *>(), nullptr);
}


ShiftTime::ShiftTime(const QVector<dive *> &changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(tr("shift time of %n dives", "", changedDives.count()));
}

void ShiftTime::redoit()
{
	for (dive *d: diveList)
		d->when += timeChanged;

	// Changing times may have unsorted the dive table
	sort_dive_table(&dive_table);
	sort_trip_table(&trip_table);

	// We send one time changed signal per trip (see comments in DiveListNotifier.h).
	// Therefore, collect all dives in an array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(diveList.size());
	for (dive *d: diveList)
		dives.push_back({ d->divetrip, d });

	// Send signals and sort tables.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		if (trip)
			sort_dive_table(&trip->dives); // Keep the trip-table in order
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

void ShiftTime::undoit()
{
	// Same as redoit(), since after redoit() we reversed the timeOffset
	redoit();
}


RenumberDives::RenumberDives(const QVector<QPair<dive *, int>> &divesToRenumberIn) : divesToRenumber(divesToRenumberIn)
{
	setText(tr("renumber %n dive(s)", "", divesToRenumber.count()));
}

void RenumberDives::undoit()
{
	renumberDives(divesToRenumber);
	mark_divelist_changed(true);
}

bool RenumberDives::workToBeDone()
{
	return !divesToRenumber.isEmpty();
}

void RenumberDives::redoit()
{
	// Redo and undo do the same thing!
	undoit();
}

bool TripBase::workToBeDone()
{
	return !divesToMove.divesToMove.empty();
}

void TripBase::redoit()
{
	moveDivesBetweenTrips(divesToMove);
	sort_trip_table(&trip_table); // Though unlikely, moving dives may reorder trips

	mark_divelist_changed(true);
}

void TripBase::undoit()
{
	// Redo and undo do the same thing!
	redoit();
}

RemoveDivesFromTrip::RemoveDivesFromTrip(const QVector<dive *> &divesToRemove)
{
	setText(tr("remove %n dive(s) from trip", "", divesToRemove.size()));
	divesToMove.divesToMove.reserve(divesToRemove.size());
	for (dive *d: divesToRemove) {
		// If a user manually removes a dive from a trip, don't autogroup this dive.
		// The flag will not be reset on undo, but that should be acceptable.
		d->notrip = true;
		divesToMove.divesToMove.push_back( {d, nullptr} );
	}
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
	for(int i = 0; (trip = get_dives_to_autogroup(&dive_table, i, &from, &to, &alloc)) != NULL; i = to) {
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
	dive_trip *newTrip = combine_trips(trip1, trip2);
	divesToMove.tripsToAdd.emplace_back(newTrip);
	for (int i = 0; i < trip1->dives.nr; ++i)
		divesToMove.divesToMove.push_back( { trip1->dives.dives[i], newTrip } );
	for (int i = 0; i < trip2->dives.nr; ++i)
		divesToMove.divesToMove.push_back( { trip2->dives.dives[i], newTrip } );
}

// std::array<dive *, 2> is the same as struct *dive[2], with the fundamental
// difference that it can be returned from functions. Thus, this constructor
// can be chained with the result of a function.
SplitDivesBase::SplitDivesBase(dive *d, std::array<dive *, 2> newDives)
{
	// If either of the new dives is null, simply return. Empty arrays indicate that nothing is to be done.
	if (!newDives[0] || !newDives[1])
		return;

	// Currently, the core code selects the dive -> this is not what we want, as
	// we manually manage the selection post-command.
	// TODO: Reset selection in core.
	newDives[0]->selected = false;
	newDives[1]->selected = false;

	// Getting the insertion indexes correct is actually not easy, as we don't know
	// which of the dives will land first when splitting out dive computers!
	// TODO: We really should think about not storing the insertion index in the undo
	// command, but calculating it on the fly on execution.
	int idx_old = get_divenr(d);
	int idx1 = dive_table_get_insertion_index(&dive_table, newDives[0]);
	int idx2 = dive_table_get_insertion_index(&dive_table, newDives[1]);
	if (idx1 > idx_old)
		--idx1;
	if (idx2 > idx_old)
		--idx2;
	if (idx1 == idx2 && dive_less_than(newDives[0], newDives[1]))
		++idx2;

	diveToSplit.dives.push_back(d);
	splitDives.dives.resize(2);
	splitDives.dives[0].dive.reset(newDives[0]);
	splitDives.dives[0].trip = d->divetrip;
	splitDives.dives[0].idx = idx1;
	splitDives.dives[1].dive.reset(newDives[1]);
	splitDives.dives[1].trip = d->divetrip;
	splitDives.dives[1].idx = idx2;
}

bool SplitDivesBase::workToBeDone()
{
	return !diveToSplit.dives.empty();
}

void SplitDivesBase::redoit()
{
	divesToUnsplit = addDives(splitDives);
	unsplitDive = removeDives(diveToSplit);
	mark_divelist_changed(true);

	// Select split dives and make first dive current
	restoreSelection(divesToUnsplit.dives, divesToUnsplit.dives[0]);
}

void SplitDivesBase::undoit()
{
	// Note: reverse order with respect to redoit()
	diveToSplit = addDives(unsplitDive);
	splitDives = removeDives(divesToUnsplit);
	mark_divelist_changed(true);

	// Select unsplit dive and make it current
	restoreSelection(diveToSplit.dives, diveToSplit.dives[0] );
}

static std::array<dive *, 2> doSplitDives(const dive *d, duration_t time)
{
	// Split the dive
	dive *new1, *new2;
	if (time.seconds < 0)
		split_dive(d, &new1, &new2);
	else
		split_dive_at_time(d, time, &new1, &new2);

	return { new1, new2 };
}

SplitDives::SplitDives(dive *d, duration_t time) : SplitDivesBase(d, doSplitDives(d, time))
{
	setText(tr("split dive"));
}

static std::array<dive *, 2> splitDiveComputer(const dive *d, int dc_num)
{
	// Refuse to do anything if the dive has only one dive computer.
	// Yes, this should have been checked by the UI, but let's just make sure.
	if (!d->dc.next)
		return { nullptr, nullptr};

	dive *new1, *new2;
	split_divecomputer(d, dc_num, &new1, &new2);

	return { new1, new2 };
}

SplitDiveComputer::SplitDiveComputer(dive *d, int dc_num) : SplitDivesBase(d, splitDiveComputer(d, dc_num))
{
	setText(tr("split dive computer"));
}

MergeDives::MergeDives(const QVector <dive *> &dives)
{
	setText(tr("merge dive"));

	// Just a safety check - if there's not two or more dives - do nothing
	// The caller should have made sure that this doesn't happen.
	if (dives.count() < 2) {
		qWarning() << "Merging less than two dives";
		return;
	}

	dive_trip *preferred_trip;
	dive_site *preferred_site;
	OwningDivePtr d(merge_dives(dives[0], dives[1], dives[1]->when - dives[0]->when, false, &preferred_trip, &preferred_site));

	// Currently, the core code selects the dive -> this is not what we want, as
	// we manually manage the selection post-command.
	// TODO: Remove selection code from core.
	d->selected = false;

	// Set the preferred dive trip, so that for subsequent merges the better trip can be selected
	d->divetrip = preferred_trip;
	for (int i = 2; i < dives.count(); ++i) {
		d.reset(merge_dives(d.get(), dives[i], dives[i]->when - d->when, false, &preferred_trip, &preferred_site));
		// Set the preferred dive trip and site, so that for subsequent merges the better trip and site can be selected
		d->divetrip = preferred_trip;
		d->dive_site = preferred_site;
	}

	// We got our preferred trip and site, so now the references can be deleted from the newly generated dive
	d->divetrip = nullptr;
	d->dive_site = nullptr;

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
			divesToRenumber.append(QPair<dive *,int>(dive_table.dives[i], newnr));
			previousnr = newnr;
		}
	}

	mergedDive.dives.resize(1);
	mergedDive.dives[0].dive = std::move(d);
	mergedDive.dives[0].idx = get_divenr(dives[0]);
	mergedDive.dives[0].trip = preferred_trip;
	divesToMerge.dives = dives.toStdVector();
}

bool MergeDives::workToBeDone()
{
	return !mergedDive.dives.empty();
}

void MergeDives::redoit()
{
	renumberDives(divesToRenumber);
	diveToUnmerge = addDives(mergedDive);
	unmergedDives = removeDives(divesToMerge);

	// Select merged dive and make it current
	restoreSelection(diveToUnmerge.dives, diveToUnmerge.dives[0]);
}

void MergeDives::undoit()
{
	divesToMerge = addDives(unmergedDives);
	mergedDive = removeDives(diveToUnmerge);
	renumberDives(divesToRenumber);

	// Select unmerged dives and make first one current
	restoreSelection(divesToMerge.dives, divesToMerge.dives[0]);
}

} // namespace Command
