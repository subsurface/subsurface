// SPDX-License-Identifier: GPL-2.0
// Helper functions for the undo-commands

#include "selection.h"
#include "divelist.h"
#include "trip.h"
#include "subsurface-qt/divelistnotifier.h"

#include <QVector>

int amount_selected;
static int amount_trips_selected;

extern "C" void select_dive(struct dive *dive)
{
	if (!dive)
		return;
	if (!dive->selected) {
		dive->selected = 1;
		amount_selected++;
	}
	current_dive = dive;
}

extern "C" void deselect_dive(struct dive *dive)
{
	int idx;
	if (dive && dive->selected) {
		dive->selected = 0;
		if (amount_selected)
			amount_selected--;
		if (current_dive == dive && amount_selected > 0) {
			/* pick a different dive as selected */
			int selected_dive = idx = get_divenr(dive);
			while (--selected_dive >= 0) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected) {
					current_dive = dive;
					return;
				}
			}
			selected_dive = idx;
			while (++selected_dive < dive_table.nr) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected) {
					current_dive = dive;
					return;
				}
			}
		}
		current_dive = NULL;
	}
}

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

// Set the current dive either from a list of selected dives,
// or a newly selected dive. In both cases, try to select the
// dive that is newer that is newer than the given date.
// This mimics the old behavior when the current dive changed.
// If a current dive outside of the selection was set, add
// it to the list of selected dives, so that we never end up
// in a situation where we display a non-selected dive.
static void setClosestCurrentDive(timestamp_t when, const std::vector<dive *> &selection, QVector<dive *> &divesToSelect)
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
	if (current_dive)
		divesToSelect.push_back(current_dive);
}

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "seletion" is empty). Return true if the selection or current dive changed.
void setSelection(const std::vector<dive *> &selection, dive *currentDive)
{
	// To do so, generate vectors of dives to be selected and deselected.
	// We send signals batched by trip, so keep track of trip/dive pairs.
	QVector<dive *> divesToSelect;
	divesToSelect.reserve(selection.size());

	// Since we select only dives, there are no selected trips!
	amount_trips_selected = 0;
	for (int i = 0; i < trip_table.nr; ++i)
		trip_table.trips[i]->selected = false;

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
		// TODO: Instead of using select_dive() and deselect_dive(), we set selected directly.
		// The reason is that deselect() automatically sets a new current dive, which we
		// don't want, as we set it later anyway.
		// There is other parts of the C++ code that touches the innards directly, but
		// ultimately this should be pushed down to C.
		d->selected = newState;
	}

	// We cannot simply change the current dive to the given dive.
	// It might be hidden by a filter and thus not be selected.
	current_dive = currentDive;
	if (current_dive && !currentDive->selected) {
		// Current not visible -> find a different dive.
		setClosestCurrentDive(currentDive->when, selection, divesToSelect);
	}

	// Send the new selection
	emit diveListNotifier.divesSelected(divesToSelect);
}

extern "C" void select_single_dive(dive *d)
{
	if (d)
		setSelection(std::vector<dive *>{ d }, d);
	else
		setSelection(std::vector<dive *>(), nullptr);
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

// Select the first dive that is visible
extern "C" void select_newest_visible_dive()
{
	for (int i = dive_table.nr - 1; i >= 0; --i) {
		dive *d = dive_table.dives[i];
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
	for (int i = 0; i < trip_table.nr; ++i) {
		if (trip_table.trips[i]->selected)
			return trip_table.trips[i];
	}
	fprintf(stderr, "warning: found no selected trip even though one should be selected\n");
	return NULL; // shouldn't happen
}

extern "C" void clear_selection(void)
{
	current_dive = nullptr;
	amount_selected = 0;
	amount_trips_selected = 0;
	int i;
	struct dive *dive;
	for_each_dive (i, dive)
		dive->selected = false;
	for (int i = 0; i < trip_table.nr; ++i)
		trip_table.trips[i]->selected = false;
}
