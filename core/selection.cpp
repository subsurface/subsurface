// SPDX-License-Identifier: GPL-2.0
// Helper functions for the undo-commands

#include "selection.h"
#include "divelist.h"
#include "divelog.h"
#include "errorhelper.h"
#include "trip.h"
#include "subsurface-qt/divelistnotifier.h"

#include <QVector>

struct dive *current_dive = NULL;
int amount_selected;
static int amount_trips_selected;

extern "C" struct dive *first_selected_dive()
{
	int idx;
	struct dive *d;

	for_each_dive (idx, d) {
		if (d->selected)
			return d;
	}
	return NULL;
}

extern "C" struct dive *last_selected_dive()
{
	int idx;
	struct dive *d, *ret = NULL;

	for_each_dive (idx, d) {
		if (d->selected)
			ret = d;
	}
	return ret;
}

extern "C" bool consecutive_selected()
{
	struct dive *d;
	int i;
	bool consecutive = true;
	bool firstfound = false;
	bool lastfound = false;

	if (amount_selected == 0 || amount_selected == 1)
		return true;

	for_each_dive(i, d) {
		if (d->selected) {
			if (!firstfound)
				firstfound = true;
			else if (lastfound)
				consecutive = false;
		} else if (firstfound) {
			lastfound = true;
		}
	}
	return consecutive;
}

#if DEBUG_SELECTION_TRACKING
extern "C" void dump_selection(void)
{
	int i;
	struct dive *dive;

	printf("currently selected are %u dives:", amount_selected);
	for_each_dive(i, dive) {
		if (dive->selected)
			printf(" %d", i);
	}
	printf("\n");
}
#endif

// Get closest dive in selection, if possible a newer dive.
// Supposes that selection is sorted
static dive *closestInSelection(timestamp_t when, const std::vector<dive *> &selection)
{
	// Start from back until we get the first dive that is before
	// the supposed-to-be selected dive. (Note: this mimics the
	// old behavior when the current dive changed).
	for (auto it = selection.rbegin(); it < selection.rend(); ++it) {
		if ((*it)->when > when && !(*it)->hidden_by_filter)
			return *it;
	}

	// We didn't find a more recent selected dive -> try to
	// find *any* visible selected dive.
	for (dive *d: selection) {
		if (!d->hidden_by_filter)
			return d;
	}

	return nullptr;
}

// Set the current dive either from a list of selected dives,
// or a newly selected dive. In both cases, try to select the
// dive that is newer than the given date.
// This mimics the old behavior when the current dive changed.
// If a current dive outside of the selection was set, add
// it to the list of selected dives, so that we never end up
// in a situation where we display a non-selected dive.
static void setClosestCurrentDive(timestamp_t when, const std::vector<dive *> &selection, QVector<dive *> &divesToSelect)
{
	if (dive *d = closestInSelection(when, selection)) {
		current_dive = d;
		return;
	}

	// No selected dive is visible! Take the closest dive. Note, this might
	// return null, but that just means unsetting the current dive (as no
	// dive is visible anyway).
	current_dive = find_next_visible_dive(when);
	if (current_dive) {
		current_dive->selected = true;
		amount_selected++;
		divesToSelect.push_back(current_dive);
	}
}


// Reset the selection to the dives of the "selection" vector.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "selection" is empty).
// Does not send signals or clear the trip selection.
QVector<dive *> setSelectionCore(const std::vector<dive *> &selection, dive *currentDive)
{
	// To do so, generate vectors of dives to be selected and deselected.
	// We send signals batched by trip, so keep track of trip/dive pairs.
	QVector<dive *> divesToSelect;
	divesToSelect.reserve(selection.size());

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

		if (newState) {
			++amount_selected;
			divesToSelect.push_back(d);
		}
		d->selected = newState;
	}

	// We cannot simply change the current dive to the given dive.
	// It might be hidden by a filter and thus not be selected.
	current_dive = currentDive;
	if (current_dive && !currentDive->selected) {
		// Current not visible -> find a different dive.
		setClosestCurrentDive(currentDive->when, selection, divesToSelect);
	}

	return divesToSelect;
}

static void clear_trip_selection()
{
	amount_trips_selected = 0;
	for (int i = 0; i < divelog.trips->nr; ++i)
		divelog.trips->trips[i]->selected = false;
}

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "selection" is empty).
// If "currentDc" is negative, an attempt will be made to keep the current computer number.
void setSelection(const std::vector<dive *> &selection, dive *currentDive, int currentDc)
{
	// Since we select only dives, there are no selected trips!
	clear_trip_selection();

	auto selectedDives = setSelectionCore(selection, currentDive);

	// Send the new selection to the UI.
	emit diveListNotifier.divesSelected(selectedDives, current_dive, currentDc);
}

// Set selection, but try to keep the current dive. If current dive is not in selection,
// find the nearest current dive in the selection
// Returns true if the current dive changed.
// Do not send a signal.
bool setSelectionKeepCurrent(const std::vector<dive *> &selection)
{
	// Since we select only dives, there are no selected trips!
	clear_trip_selection();

	const dive *oldCurrent = current_dive;

	dive *newCurrent = current_dive;
	if (current_dive && std::find(selection.begin(), selection.end(), current_dive) == selection.end())
		newCurrent = closestInSelection(current_dive->when, selection);
	setSelectionCore(selection, newCurrent);

	return current_dive != oldCurrent;
}

void setTripSelection(dive_trip *trip, dive *currentDive)
{
	if (!trip)
		return;

	current_dive = currentDive;
	for (int i = 0; i < divelog.dives->nr; ++i) {
		dive &d = *divelog.dives->dives[i];
		d.selected = d.divetrip == trip;
	}
	for (int i = 0; i < divelog.trips->nr; ++i) {
		dive_trip *t = divelog.trips->trips[i];
		t->selected = t == trip;
	}

	amount_selected = trip->dives.nr;
	amount_trips_selected = 1;

	emit diveListNotifier.tripSelected(trip, currentDive);
}

extern "C" void select_single_dive(dive *d)
{
	if (d)
		setSelection(std::vector<dive *>{ d }, d, -1);
	else
		setSelection(std::vector<dive *>(), nullptr, -1);
}

// Turn current selection into a vector.
// TODO: This could be made much more efficient if we kept a sorted list of selected dives!
std::vector<dive *> getDiveSelection()
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

bool diveInSelection(const std::vector<dive *> &selection, const dive *d)
{
	// Do a binary search using the ordering of the dive list.
	auto it = std::lower_bound(selection.begin(), selection.end(), d, dive_less_than);
	return it != selection.end() && *it == d;
}

void updateSelection(std::vector<dive *> &selection, const std::vector<dive *> &add, const std::vector<dive *> &remove)
{
	// We could sort the array and merge the vectors as we do in the undo code. But is it necessary?
	for (dive *d: add) {
		auto it = std::lower_bound(selection.begin(), selection.end(), d, dive_less_than);
		if (it != selection.end() && *it == d)
			continue; // Ooops. Already there?
		selection.insert(it, d);
	}

	// Likewise, we could sort the array and be smarter here. Again, is it necessary?
	for (dive *d: remove) {
		auto it = std::lower_bound(selection.begin(), selection.end(), d, dive_less_than);
		if (it == selection.end() || *it != d)
			continue; // Ooops. Not there?
		selection.erase(it);
	}
}

// Select the first dive that is visible
extern "C" void select_newest_visible_dive()
{
	for (int i = divelog.dives->nr - 1; i >= 0; --i) {
		dive *d = divelog.dives->dives[i];
		if (!d->hidden_by_filter)
			return select_single_dive(d);
	}

	// No visible dive -> deselect all
	select_single_dive(nullptr);
}

extern "C" void select_trip(struct dive_trip *trip)
{
	if (trip && !trip->selected) {
		trip->selected = true;
		amount_trips_selected++;
	}
}

extern "C" void deselect_trip(struct dive_trip *trip)
{
	if (trip && trip->selected) {
		trip->selected = false;
		amount_trips_selected--;
	}
}

extern "C" struct dive_trip *single_selected_trip()
{
	if (amount_trips_selected != 1)
		return NULL;
	for (int i = 0; i < divelog.trips->nr; ++i) {
		if (divelog.trips->trips[i]->selected)
			return divelog.trips->trips[i];
	}
	report_info("warning: found no selected trip even though one should be selected");
	return NULL; // shouldn't happen
}
