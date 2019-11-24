// SPDX-License-Identifier: GPL-2.0
// Selection related functions

#ifndef SELECTION_H
#define SELECTION_H

/*** C++-only functions ***/

#ifdef __cplusplus
#include <vector>

struct dive;

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "seletion" is empty).
void setSelection(const std::vector<dive *> &selection, dive *currentDive);

// Get currently selectd dives
std::vector<dive *> getDiveSelection();

#endif // __cplusplus

#endif // SELECTION_H
