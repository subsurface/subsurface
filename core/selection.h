// SPDX-License-Identifier: GPL-2.0
// Selection related functions

#ifndef SELECTION_H
#define SELECTION_H

#include <vector>
#include <QVector>

struct dive;

extern int amount_selected;
extern struct dive *current_dive;

extern struct dive *first_selected_dive();
extern struct dive *last_selected_dive();
extern bool consecutive_selected();
extern void select_newest_visible_dive();
extern void select_single_dive(struct dive *d); // wrapper for setSelection() with a single dive. NULL clears the selection.
extern void select_trip(struct dive_trip *trip);
extern void deselect_trip(struct dive_trip *trip);
extern struct dive_trip *single_selected_trip(); // returns trip if exactly one trip is selected, NULL otherwise.
extern void clear_selection();

#if DEBUG_SELECTION_TRACKING
extern void dump_selection();
#endif

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive" and the current dive computer to "currentDc".
// "currentDive" must be an element of "selection" (or null if "seletion" is empty).
// If "currentDc" is negative, an attempt will be made to keep the current computer number.
// Returns the list of selected dives
QVector<dive *> setSelectionCore(const std::vector<dive *> &selection, dive *currentDive);

// As above, but sends a signal to inform the frontend of the changed selection.
// Returns true if the current dive changed.
void setSelection(const std::vector<dive *> &selection, dive *currentDive, int currentDc);

// Set selection, but try to keep the current dive. If current dive is not in selection,
// find the nearest current dive in the selection
// Returns true if the current dive changed.
// Does not send a signal.
bool setSelectionKeepCurrent(const std::vector<dive *> &selection);

// Select all dive in a trip and sends a trip-selected signal
void setTripSelection(dive_trip *trip, dive *currentDive);

// Get currently selected dives
std::vector<dive *> getDiveSelection();
bool diveInSelection(const std::vector<dive *> &selection, const dive *d);
void updateSelection(std::vector<dive *> &selection, const std::vector<dive *> &add, const std::vector<dive *> &remove);

#endif // SELECTION_H
