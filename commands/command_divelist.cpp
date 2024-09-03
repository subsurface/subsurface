// SPDX-License-Identifier: GPL-2.0

#include "command_divelist.h"
#include "core/divefilter.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/qthelper.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/filtermodels.h"
#include "qt-models/divelocationmodel.h"

namespace Command {

// Helper function that takes care to unselect trips that are removed from the backend
static std::unique_ptr<dive_trip> remove_trip_from_backend(dive_trip *trip)
{
	if (trip->selected)
		deselect_trip(trip);
	auto [t, idx] = divelog.trips.pull(trip);
	return std::move(t);
}

// This helper function removes a dive, takes ownership of the dive and adds it to a DiveToAdd structure.
// If the trip the dive belongs to becomes empty, it is removed and added to the tripsToAdd vector.
// It is crucial that dives are added in reverse order of deletion, so that the indices are correctly
// set and that the trips are added before they are used!
DiveToAdd DiveListBase::removeDive(struct dive *d, std::vector<std::unique_ptr<dive_trip>> &tripsToAdd)
{
	// If the dive was the current dive, reset the current dive. The calling
	// command is responsible of finding a new dive.
	if (d == current_dive)
		current_dive = nullptr;

	// remove dive from trip and site - if this is the last dive in the trip
	// remove the whole trip.
	DiveToAdd res;
	res.trip = unregister_dive_from_trip(d);
	if (d->dive_site)
		diveSiteCountChanged(d->dive_site);
	res.site = unregister_dive_from_dive_site(d);
	if (res.trip && res.trip->dives.empty()) {
		divelog.trips.sort();				// Removal of dives has changed order of trips! (TODO: remove this)
		auto trip = remove_trip_from_backend(res.trip);	// Remove trip from backend
		tripsToAdd.push_back(std::move(trip));		// Take ownership of trip
	}

	size_t idx = divelog.dives.get_idx(d);
	if (idx == std::string::npos)
		qWarning("Deletion of unknown dive!");

	DiveFilter::instance()->diveRemoved(d);

	res.dive = divelog.dives.unregister_dive(idx);		// Remove dive from backend

	return res;
}

void DiveListBase::diveSiteCountChanged(struct dive_site *ds)
{
	if (std::find(sitesCountChanged.begin(), sitesCountChanged.end(), ds) == sitesCountChanged.end())
		sitesCountChanged.push_back(ds);
}

// This helper function adds a dive and returns ownership to the backend. It may also add a dive trip.
// It is crucial that dives are added in reverse order of deletion (see comment above)!
// Returns pointer to added dive (which is owned by the backend!)
dive *DiveListBase::addDive(DiveToAdd &d)
{
	if (d.trip)
		d.trip->add_dive(d.dive.get());
	if (d.site) {
		d.site->add_dive(d.dive.get());
		diveSiteCountChanged(d.site);
	}
	return divelog.dives.register_dive(std::move(d.dive));	// Transfer ownership to core and update fulltext index
}

// Some signals are sent in batches per trip. To avoid writing the same loop
// twice, this template takes a vector of trip / dive pairs, sorts it
// by trip and then calls a function-object with trip and a QVector of dives in that trip.
// The dives are sorted by the dive_less_than() function defined in the core.
// Input parameters:
//	- dives: a vector of trip,dive pairs, which will be sorted and processed in batches by trip.
//	- action: a function object, taking a trip-pointer and a QVector of dives, which will be called for each batch.
template<typename Function>
void processByTrip(std::vector<std::pair<dive_trip *, dive *>> &dives, Function action)
{
	// Sort lexicographically by trip then according to the dive_less_than() function.
	std::sort(dives.begin(), dives.end(),
		  [](const std::pair<dive_trip *, dive *> &e1, const std::pair<dive_trip *, dive *> &e2)
		  { return e1.first == e2.first ? dive_less_than(*e1.second, *e2.second) : e1.first < e2.first; });

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

// This helper function calls removeDive() on a list of dives to be removed and
// returns a vector of corresponding DiveToAdd objects, which can later be readded.
// Moreover, a vector of deleted trips is returned, if trips became empty.
// The passed in vector is cleared.
DivesAndTripsToAdd DiveListBase::removeDives(DivesAndSitesToRemove &divesAndSitesToDelete)
{
	std::vector<DiveToAdd> divesToAdd;
	std::vector<std::unique_ptr<dive_trip>> tripsToAdd;
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
	divesToAdd.reserve(divesAndSitesToDelete.dives.size());
	sitesToAdd.reserve(divesAndSitesToDelete.sites.size());

	// Remember old number of shown dives
	int oldShown = DiveFilter::instance()->shownDives();

	// Make sure that the dive list is sorted. The added dives will be sent in a signal
	// and the recipients assume that the dives are sorted the same way as they are
	// in the core list.
	std::sort(divesAndSitesToDelete.dives.begin(), divesAndSitesToDelete.dives.end(),
		  [](const dive *d1, const dive *d2) { return dive_less_than(*d1, *d2); });

	for (dive *d: divesAndSitesToDelete.dives)
		divesToAdd.push_back(removeDive(d, tripsToAdd));
	divesAndSitesToDelete.dives.clear();

	for (dive_site *ds: divesAndSitesToDelete.sites) {
		auto res = divelog.sites.pull(ds);
		sitesToAdd.push_back(std::move(res.ptr));
		emit diveListNotifier.diveSiteDeleted(ds, res.idx);
	}
	divesAndSitesToDelete.sites.clear();

	// We send one dives-deleted signal per trip (see comments in divelistnotifier.h).
	// Therefore, collect all dives in an array and sort by trip.
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(divesToAdd.size());
	for (const DiveToAdd &entry: divesToAdd)
		dives.push_back({ entry.trip, entry.dive.get() });

	// Send signals.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Check if this trip is supposed to be deleted, by checking if it was marked as "add it".
		bool deleteTrip = trip &&
				  std::find_if(tripsToAdd.begin(), tripsToAdd.end(), [trip](const std::unique_ptr<dive_trip> &ptr)
					       { return ptr.get() == trip; }) != tripsToAdd.end();
		emit diveListNotifier.divesDeleted(trip, deleteTrip, divesInTrip);
	});

	if (oldShown != DiveFilter::instance()->shownDives())
		emit diveListNotifier.numShownChanged();

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
	std::vector<std::pair<dive_trip *, dive *>> dives;
	res.resize(toAdd.dives.size());
	sites.reserve(toAdd.sites.size());
	dives.reserve(toAdd.sites.size());

	// Make sure that the dive list is sorted. The added dives will be sent in a signal
	// and the recipients assume that the dives are sorted the same way as they are
	// in the core list.
	std::sort(toAdd.dives.begin(), toAdd.dives.end(),
		  [](const DiveToAdd &d, const DiveToAdd &d2)
		  { return dive_less_than(*d.dive, *d2.dive); });

	// Now, add the dives
	// Note: the idiomatic STL-way would be std::transform, but let's use a loop since
	// that is closer to classical C-style.
	auto it2 = res.rbegin();
	QVector<dive *> divesToFilter;
	divesToFilter.reserve(toAdd.dives.size());
	for (auto it = toAdd.dives.rbegin(); it != toAdd.dives.rend(); ++it, ++it2) {
		*it2 = addDive(*it);
		dives.push_back({ (*it2)->divetrip, *it2 });
		divesToFilter.push_back(*it2);
	}
	toAdd.dives.clear();

	ShownChange change = DiveFilter::instance()->update(divesToFilter);

	// If the dives belong to new trips, add these as well.
	// Remember the pointers so that we can later check if a trip was newly added
	std::vector<dive_trip *> addedTrips;
	addedTrips.reserve(toAdd.trips.size());
	for (std::unique_ptr<dive_trip> &trip: toAdd.trips) {
		auto [t, idx] = divelog.trips.put(std::move(trip)); // Return ownership to backend
		addedTrips.push_back(t);
	}
	toAdd.trips.clear();

	// Finally, add any necessary dive sites
	for (std::unique_ptr<dive_site> &ds: toAdd.sites) {
		auto res = divelog.sites.register_site(std::move(ds));
		sites.push_back(res.ptr);
		emit diveListNotifier.diveSiteAdded(sites.back(), res.idx);
	}
	toAdd.sites.clear();

	// Send signals by trip.
	processByTrip(dives, [&](dive_trip *trip, const QVector<dive *> &divesInTrip) {
		// Now, let's check if this trip is supposed to be created, by checking if it was marked as "add it".
		bool createTrip = trip && std::find(addedTrips.begin(), addedTrips.end(), trip) != addedTrips.end();
		// Finally, emit the signal
		emit diveListNotifier.divesAdded(trip, createTrip, divesInTrip);
	});

	if (!change.newShown.empty() || !change.newHidden.empty())
		emit diveListNotifier.numShownChanged();

	return { std::move(res), std::move(sites) };
}

// This helper function renumbers dives according to an array of id/number pairs.
// The old numbers are stored in the array, thus calling this function twice has no effect.
static void renumberDives(QVector<QPair<dive *, int>> &divesToRenumber)
{
	QVector<dive *> dives;
	dives.reserve(divesToRenumber.size());
	for (auto &pair: divesToRenumber) {
		dive *d = pair.first;
		if (!d)
			continue;
		std::swap(d->number, pair.second);
		dives.push_back(d);
		d->invalidate_cache();
	}

	// Send signals.
	emit diveListNotifier.divesChanged(dives, DiveField::NR);
}

// This helper function moves a dive to a trip. The old trip is recorded in the
// passed-in structure. This means that calling the function twice on the same
// object is a no-op concerning the dive. If the old trip was deleted from the
// core, an owning pointer to the removed trip is returned, otherwise a null pointer.
static std::unique_ptr<dive_trip> moveDiveToTrip(DiveToTrip &diveToTrip)
{
	// Firstly, check if we move to the same trip and bail if this is a no-op.
	if (diveToTrip.trip == diveToTrip.dive->divetrip)
		return {};

	// Remove from old trip
	std::unique_ptr<dive_trip> res;

	// Remove dive from trip - if this is the last dive in the trip, remove the whole trip.
	dive_trip *trip = unregister_dive_from_trip(diveToTrip.dive);
	if (trip && trip->dives.empty())
		res = remove_trip_from_backend(trip);	// Remove trip from backend

	// Store old trip and get new trip we should associate this dive with
	std::swap(trip, diveToTrip.trip);
	if (trip)
		trip->add_dive(diveToTrip.dive);
	diveToTrip.dive->invalidate_cache();		// Ensure that dive is written in git_save()
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
	for (std::unique_ptr<dive_trip> &trip: dives.tripsToAdd) {
		auto [t, idx] = divelog.trips.put(std::move(trip)); // Return ownership to backend
		createdTrips.push_back(t);
	}
	dives.tripsToAdd.clear();

	for (DiveToTrip &dive: dives.divesToMove) {
		std::unique_ptr<dive_trip> tripToAdd = moveDiveToTrip(dive);
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
					       [from](const std::unique_ptr<dive_trip> &trip) { return trip.get() == from; }) != dives.tripsToAdd.end();
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

void DiveListBase::initWork()
{
}

void DiveListBase::finishWork()
{
	for (dive_site *ds: sitesCountChanged)
		emit diveListNotifier.diveSiteDiveCountChanged(ds);
}

void DiveListBase::undo()
{
	initWork();
	undoit();
	finishWork();
}

void DiveListBase::redo()
{
	initWork();
	redoit();
	finishWork();
}

AddDive::AddDive(std::unique_ptr<dive> d, bool autogroup, bool newNumber)
{
	setText(Command::Base::tr("add dive"));
	d->maxdepth = 0_m;
	d->dcs[0].maxdepth = 0_m;
	divelog.dives.fixup_dive(*d);

	// this only matters if undoit were called before redoit
	currentDive = nullptr;

	d->selected = false; // If we clone a planned dive, it might have been selected.
			     // We have to clear the flag, as selections will be managed
			     // on dive-addition.

	// If we alloc a new-trip for autogrouping, get an owning pointer to it.
	std::unique_ptr<dive_trip> allocTrip;
	dive_trip *trip = d->divetrip;
	dive_site *site = d->dive_site;
	// We have to delete the pointers to trip and site, because this would prevent the core from adding to the
	// trip or site and we would get the count-of-dives in the trip or site wrong. Yes, that's all horribly subtle!
	d->divetrip = nullptr;
	d->dive_site = nullptr;
	if (!trip && autogroup) {
		auto [t, allocated] = get_trip_for_new_dive(divelog, d.get());
		trip = t;
		allocTrip = std::move(allocated);
	}

	int idx = divelog.dives.get_insertion_index(d.get());
	if (newNumber)
		d->number = divelog.dives.get_dive_nr_at_idx(idx);

	divesToAdd.dives.push_back({ std::move(d), trip, site });
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
	divelog.trips.sort(); // Though unlikely, adding a dive may reorder trips

	// Select the newly added dive
	setSelection(divesAndSitesToRemove.dives, divesAndSitesToRemove.dives[0], -1);
}

void AddDive::undoit()
{
	// Simply remove the dive that was previously added...
	divesToAdd = removeDives(divesAndSitesToRemove);
	divelog.trips.sort(); // Though unlikely, removing a dive may reorder trips

	// ...and restore the selection
	setSelection(selection, currentDive, -1);
}

ImportDives::ImportDives(struct divelog *log, int flags, const QString &source)
{
	setText(Command::Base::tr("import %n dive(s) from %1", "", log->dives.size()).arg(source));

	// this only matters if undoit were called before redoit
	currentDive = nullptr;

	auto [dives_to_add, dives_to_remove, trips_to_add, sites_to_add, devices_to_add] =
		divelog.process_imported_dives(*log, flags);

	// Add devices to devicesToAddAndRemove structure
	devicesToAddAndRemove = std::move(devices_to_add);

	// Add trips to the divesToAdd.trips structure
	divesToAdd.trips.reserve(trips_to_add.size());
	for (auto &trip: trips_to_add)
		divesToAdd.trips.push_back(std::move(trip));

	// Add sites to the divesToAdd.sites structure
	divesToAdd.sites = std::move(sites_to_add);

	// Add dives to the divesToAdd.dives structure
	divesToAdd.dives.reserve(dives_to_add.size());
	for (auto &divePtr: dives_to_add) {
		divePtr->selected = false; // See above in AddDive::AddDive()
		dive_trip *trip = divePtr->divetrip;
		divePtr->divetrip = nullptr; // See above in AddDive::AddDive()
		dive_site *site = divePtr->dive_site;
		divePtr->dive_site = nullptr; // See above in AddDive::AddDive()

		divesToAdd.dives.push_back({ std::move(divePtr), trip, site });
	}

	// Add dive to be deleted to the divesToRemove structure
	divesAndSitesToRemove.dives = std::move(dives_to_remove);

	// When encountering filter presets with equal names, check whether they are
	// the same. If they are, ignore them.
	for (const filter_preset &preset: log->filter_presets) {
		std::string name = preset.name;
		auto it = std::find_if(divelog.filter_presets.begin(), divelog.filter_presets.end(),
				       [&name](const filter_preset &preset) { return preset.name == name; });
		if (it != divelog.filter_presets.end() && it->data == preset.data)
			continue;
		filterPresetsToAdd.emplace_back(preset.name, preset.data);
	}
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
	setSelection(divesAndSitesToRemoveNew.dives, divesAndSitesToRemoveNew.dives.back(), -1);

	// Remember dives and sites to remove
	divesAndSitesToRemove = std::move(divesAndSitesToRemoveNew);

	// Add devices
	for (const device &dev: devicesToAddAndRemove)
		add_to_device_table(divelog.devices, dev);

	// Add new filter presets
	for (auto &it: filterPresetsToAdd) {
		filterPresetsToRemove.push_back(divelog.filter_presets.add(it.first, it.second));
		emit diveListNotifier.filterPresetAdded(filterPresetsToRemove.back());
	}
	filterPresetsToAdd.clear();

	emit diveListNotifier.divesImported();
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
	setSelection(selection, currentDive, -1);

	// Remove devices
	for (const device &dev: devicesToAddAndRemove)
		remove_device(divelog.devices, dev);

	// Remove filter presets. Do this in reverse order.
	for (auto it = filterPresetsToRemove.rbegin(); it != filterPresetsToRemove.rend(); ++it) {
		int index = *it;
		const filter_preset &preset = divelog.filter_presets[index];
		std::string oldName = preset.name;
		FilterData oldData = preset.data;
		divelog.filter_presets.remove(index);
		emit diveListNotifier.filterPresetRemoved(index);
		filterPresetsToAdd.emplace_back(oldName, oldData);
	}
	filterPresetsToRemove.clear();
	std::reverse(filterPresetsToAdd.begin(), filterPresetsToAdd.end());
}

DeleteDive::DeleteDive(const QVector<struct dive*> &divesToDeleteIn)
{
	divesToDelete.dives = std::vector<dive *>(divesToDeleteIn.begin(), divesToDeleteIn.end());
	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("delete %n dive(s)", "", divesToDelete.dives.size())).arg(getListOfDives(divesToDelete.dives)));
}

bool DeleteDive::workToBeDone()
{
	return !divesToDelete.dives.empty();
}

void DeleteDive::undoit()
{
	divesToDelete = addDives(divesToAdd);
	divelog.trips.sort(); // Though unlikely, removing a dive may reorder trips

	// Select all re-added dives and make the first one current
	dive *currentDive = !divesToDelete.dives.empty() ? divesToDelete.dives[0] : nullptr;
	setSelection(divesToDelete.dives, currentDive, -1);
}

void DeleteDive::redoit()
{
	divesToAdd = removeDives(divesToDelete);
	divelog.trips.sort(); // Though unlikely, adding a dive may reorder trips

	// Deselect all dives and select dive that was close to the first deleted dive
	dive *newCurrent = nullptr;
	if (!divesToAdd.dives.empty()) {
		timestamp_t when = divesToAdd.dives[0].dive->when;
		newCurrent = divelog.dives.find_next_visible_dive(when);
	}
	select_single_dive(newCurrent);
}


ShiftTime::ShiftTime(std::vector<dive *> changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("shift time of %n dives", "", changedDives.size())).arg(getListOfDives(changedDives)));
}

void ShiftTime::redoit()
{
	std::vector<dive_trip *> trips;
	for (dive *d: diveList) {
		d->when += timeChanged;
		if (d->divetrip && std::find(trips.begin(), trips.end(), d->divetrip) == trips.end())
			trips.push_back(d->divetrip);
	}

	// Changing times may have unsorted the dive and trip tables
	divelog.dives.sort();
	divelog.trips.sort();
	for (dive_trip *trip: trips)
		trip->sort_dives();

	// Send signals
	QVector<dive *> dives = stdToQt<dive *>(diveList);
	emit diveListNotifier.divesTimeChanged(timeChanged, dives);
	emit diveListNotifier.divesChanged(dives, DiveField::DATETIME);

	// Select the changed dives
	setSelection(diveList, diveList[0], -1);

	// Negate the time-shift so that the next call does the reverse
	timeChanged = -timeChanged;
}

bool ShiftTime::workToBeDone()
{
	return !diveList.empty();
}

void ShiftTime::undoit()
{
	// Same as redoit(), since after redoit() we reversed the timeOffset
	redoit();
}


RenumberDives::RenumberDives(const QVector<QPair<dive *, int>> &divesToRenumberIn) : divesToRenumber(divesToRenumberIn)
{
	std::vector<struct dive *> dives;
	dives.reserve(divesToRenumber.length());
	for (QPair<dive *, int>divePlusNumber: divesToRenumber)
		dives.push_back(divePlusNumber.first);
	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("renumber %n dive(s)", "", divesToRenumber.count())).arg(getListOfDives(dives)));
}

void RenumberDives::undoit()
{
	renumberDives(divesToRenumber);

	// Select the changed dives
	std::vector<dive *> dives;
	dives.reserve(divesToRenumber.size());
	for (const QPair<dive *, int> &item: divesToRenumber)
		dives.push_back(item.first);
	setSelection(dives, dives[0], -1);
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
	divelog.trips.sort(); // Though unlikely, moving dives may reorder trips

	// Select the moved dives
	std::vector<dive *> dives;
	dives.reserve(divesToMove.divesToMove.size());
	for (const DiveToTrip &item: divesToMove.divesToMove)
		dives.push_back(item.dive);
	setSelection(dives, dives[0], -1);
}

void TripBase::undoit()
{
	// Redo and undo do the same thing!
	redoit();
}

RemoveDivesFromTrip::RemoveDivesFromTrip(const QVector<dive *> &divesToRemoveIn)
{
	// Filter out dives outside of trip. Note: This is in a separate loop to get the command-description right.
	QVector<dive *> divesToRemove;
	for (dive *d: divesToRemoveIn) {
		if (d->divetrip)
			divesToRemove.push_back(d);
	}
	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("remove %n dive(s) from trip", "", divesToRemove.size())).arg(getListOfDives(divesToRemove)));
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
	setText(Command::Base::tr("remove autogenerated trips"));
	// TODO: don't touch core-innards directly
	for (auto &d: divelog.dives) {
		if (d->divetrip && d->divetrip->autogen)
			divesToMove.divesToMove.push_back( {d.get(), nullptr} );
	}
}

AddDivesToTrip::AddDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip)
{
	setText(QStringLiteral("%1 [%2]").arg(Command::Base::tr("add %n dives to trip", "", divesToAddIn.size())).arg(getListOfDives(divesToAddIn)));
	for (dive *d: divesToAddIn)
		divesToMove.divesToMove.push_back( {d, trip} );
}

CreateTrip::CreateTrip(const QVector<dive *> &divesToAddIn)
{
	setText(Command::Base::tr("create trip"));

	if (divesToAddIn.isEmpty())
		return;

	auto trip = create_trip_from_dive(divesToAddIn[0]);
	for (dive *d: divesToAddIn)
		divesToMove.divesToMove.push_back( { d, trip.get() });
	divesToMove.tripsToAdd.push_back(std::move(trip));
}

AutogroupDives::AutogroupDives()
{
	setText(Command::Base::tr("autogroup dives"));

	for (auto &entry: get_dives_to_autogroup(divelog.dives)) {
		// If this is an allocated trip, take ownership
		if (entry.created_trip)
			divesToMove.tripsToAdd.push_back(std::move(entry.created_trip));
		for (auto it = divelog.dives.begin() + entry.from; it != divelog.dives.begin() + entry.to; ++it)
			divesToMove.divesToMove.push_back( { it->get(), entry.trip } );
	}
}

MergeTrips::MergeTrips(dive_trip *trip1, dive_trip *trip2)
{
	if (trip1 == trip2)
		return;
	std::unique_ptr<dive_trip> newTrip = combine_trips(trip1, trip2);
	for (dive *d: trip1->dives)
		divesToMove.divesToMove.push_back( { d, newTrip.get() } );
	for (dive *d: trip2->dives)
		divesToMove.divesToMove.push_back( { d, newTrip.get() } );
	divesToMove.tripsToAdd.push_back(std::move(newTrip));
}

SplitDivesBase::SplitDivesBase(dive *d, std::array<std::unique_ptr<dive>, 2> newDives)
{
	// If either of the new dives is null, simply return. Empty arrays indicate that nothing is to be done.
	if (!newDives[0] || !newDives[1])
		return;

	// Currently, the core code selects the dive -> this is not what we want, as
	// we manually manage the selection post-command.
	// TODO: Reset selection in core.
	newDives[0]->selected = false;
	newDives[1]->selected = false;

	// The new dives will be registered to the dive site using the site member
	// of the DiveToAdd structure. For this to work, we must set the dive's
	// dive_site member to null. Yes, that's subtle!
	newDives[0]->dive_site = nullptr;
	newDives[1]->dive_site = nullptr;

	diveToSplit.dives.push_back(d);
	splitDives.dives.resize(2);
	splitDives.dives[0].dive = std::move(newDives[0]);
	splitDives.dives[0].trip = d->divetrip;
	splitDives.dives[0].site = d->dive_site;
	splitDives.dives[1].dive = std::move(newDives[1]);
	splitDives.dives[1].trip = d->divetrip;
	splitDives.dives[1].site = d->dive_site;
}

bool SplitDivesBase::workToBeDone()
{
	return !diveToSplit.dives.empty();
}

void SplitDivesBase::redoit()
{
	divesToUnsplit = addDives(splitDives);
	unsplitDive = removeDives(diveToSplit);

	// Select split dives and make first dive current
	setSelection(divesToUnsplit.dives, divesToUnsplit.dives[0], -1);
}

void SplitDivesBase::undoit()
{
	// Note: reverse order with respect to redoit()
	diveToSplit = addDives(unsplitDive);
	splitDives = removeDives(divesToUnsplit);

	// Select unsplit dive and make it current
	setSelection(diveToSplit.dives, diveToSplit.dives[0], -1);
}

static std::array<std::unique_ptr<dive>, 2> doSplitDives(const dive *d, duration_t time)
{
	// Split the dive
	if (time.seconds < 0)
		return divelog.dives.split_dive(*d);
	else
		return divelog.dives.split_dive_at_time(*d, time);
}

SplitDives::SplitDives(dive *d, duration_t time) : SplitDivesBase(d, doSplitDives(d, time))
{
	setText(Command::Base::tr("split dive"));
}

SplitDiveComputer::SplitDiveComputer(dive *d, int dc_num) :
	SplitDivesBase(d, divelog.dives.split_divecomputer(*d, dc_num))
{
	setText(Command::Base::tr("split dive computer"));
}

DiveComputerBase::DiveComputerBase(dive *old_dive, std::unique_ptr<dive> new_dive, int dc_nr_before, int dc_nr_after) :
	dc_nr_before(dc_nr_before),
	dc_nr_after(dc_nr_after)
{
	if (!new_dive)
		return;

	diveToRemove.dives.push_back(old_dive);

	// Currently, the core code selects the dive -> this is not what we want, as
	// we manually manage the selection post-command.
	// TODO: Reset selection in core.
	new_dive->selected = false;

	// Reset references to trip and site in the new dive, as the undo command
	// will add the dive to the trip and site.
	new_dive->divetrip = nullptr;
	new_dive->dive_site = nullptr;

	diveToAdd.dives.resize(1);
	diveToAdd.dives[0].dive = std::move(new_dive);
	diveToAdd.dives[0].trip = old_dive->divetrip;
	diveToAdd.dives[0].site = old_dive->dive_site;
}

bool DiveComputerBase::workToBeDone()
{
	return !diveToRemove.dives.empty() || !diveToAdd.dives.empty();
}

void DiveComputerBase::redoit()
{
	DivesAndSitesToRemove addedDive = addDives(diveToAdd);
	diveToAdd = removeDives(diveToRemove);
	diveToRemove = std::move(addedDive);

	// Select added dive and make it current.
	// This automatically replots the profile.
	setSelection(diveToRemove.dives, diveToRemove.dives[0], dc_nr_after);

	std::swap(dc_nr_before, dc_nr_after);
}

void DiveComputerBase::undoit()
{
	// Undo and redo do the same
	redoit();
}

MoveDiveComputerToFront::MoveDiveComputerToFront(dive *d, int dc_num)
	: DiveComputerBase(d, clone_make_first_dc(*d, dc_num), dc_num, 0)
{
	setText(Command::Base::tr("move dive computer to front"));
}

DeleteDiveComputer::DeleteDiveComputer(dive *d, int dc_num)
	: DiveComputerBase(d, divelog.dives.clone_delete_divecomputer(*d, dc_num),
			   dc_num, std::min(d->number_of_computers() - 1, dc_num))
{
	setText(Command::Base::tr("delete dive computer"));
}

MergeDives::MergeDives(const QVector <dive *> &dives) : site(nullptr)
{
	setText(Command::Base::tr("merge dive"));

	// Just a safety check - if there's not two or more dives - do nothing.
	// The caller should have made sure that this doesn't happen.
	if (dives.count() < 2) {
		qWarning("Merging less than two dives");
		return;
	}

	auto [d, set_location] = divelog.dives.merge_dives(qtToStd(dives));

	// The merged dive gets the number of the first dive with a non-zero number
	for (const dive *dive: dives) {
		if (dive->number) {
			d->number = dive->number;
			break;
		}
	}

	// We will only renumber the remaining dives if the joined dives are consecutive.
	// Otherwise all bets are off concerning what the user wanted and doing nothing seems
	// like the best option.
	size_t idx = divelog.dives.get_idx(dives[0]);
	size_t num = dives.count();
	if (idx == std::string::npos) {
		// It was the callers responsibility to pass only known dives.
		// Something is seriously wrong - give up.
		qWarning("Merging unknown dives");
		return;
	}
	// std::equal compares two ranges. The parameters are (begin_range1, end_range1, begin_range2).
	// Here, we can compare C-arrays, because QVector guarantees contiguous storage.
	if (std::equal(&dives[0], &dives[0] + num, divelog.dives.begin() + idx, [](dive *d1,
		       const std::unique_ptr<dive> &d2) { return d1 == d2.get(); }) &&
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
		int previousnr = dives[0]->number;
		for (size_t i = idx + num; i < divelog.dives.size(); ++i) {
			int newnr = divelog.dives[i]->number - diff;

			// Stop renumbering if stuff isn't in order (see also core/divelist.c)
			if (newnr <= previousnr)
				break;
			divesToRenumber.append(QPair<dive *,int>(divelog.dives[i].get(), newnr));
			previousnr = newnr;
		}
	}

	mergedDive.dives.resize(1);
	mergedDive.dives[0].trip = d->divetrip;
	mergedDive.dives[0].site = d->dive_site;
	divesToMerge.dives = std::vector<dive *>(dives.begin(), dives.end());

	if (set_location.has_value() && d->dive_site) {
		location = *set_location;
		site = d->dive_site;
	}

	// We got our preferred trip and site, so now the references can be deleted from the newly generated dive
	d->divetrip = nullptr;
	d->dive_site = nullptr;

	mergedDive.dives[0].dive = std::move(d);
}

bool MergeDives::workToBeDone()
{
	return !mergedDive.dives.empty();
}

void MergeDives::swapDivesite()
{
	if (!site)
		return;
	std::swap(location, site->location);
	emit diveListNotifier.diveSiteChanged(site, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
}

void MergeDives::redoit()
{
	renumberDives(divesToRenumber);
	diveToUnmerge = addDives(mergedDive);
	unmergedDives = removeDives(divesToMerge);
	swapDivesite();

	// Select merged dive and make it current
	setSelection(diveToUnmerge.dives, diveToUnmerge.dives[0], -1);
}

void MergeDives::undoit()
{
	divesToMerge = addDives(unmergedDives);
	mergedDive = removeDives(diveToUnmerge);
	renumberDives(divesToRenumber);
	swapDivesite();

	// Select unmerged dives and make first one current
	setSelection(divesToMerge.dives, divesToMerge.dives[0], -1);
}

} // namespace Command
