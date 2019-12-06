// SPDX-License-Identifier: GPL-2.0
// Selection related functions

#ifndef SELECTION_H
#define SELECTION_H

struct dive;

extern int amount_selected;

/*** C and C++ functions ***/

#ifdef __cplusplus
extern "C" {
#endif

extern void select_dive(struct dive *dive);
extern void deselect_dive(struct dive *dive);
extern struct dive *first_selected_dive(void);
extern struct dive *last_selected_dive(void);
extern bool consecutive_selected(void);
extern void select_newest_visible_dive();
extern void select_single_dive(struct dive *d); // wrapper for setSelection() with a single dive. NULL clears the selection.

#if DEBUG_SELECTION_TRACKING
extern void dump_selection(void);
#endif

#ifdef __cplusplus
}
#endif

/*** C++-only functions ***/

#ifdef __cplusplus
#include <vector>

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "seletion" is empty).
void setSelection(const std::vector<dive *> &selection, dive *currentDive);

// Get currently selectd dives
std::vector<dive *> getDiveSelection();

#endif // __cplusplus

#endif // SELECTION_H
