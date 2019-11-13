// SPDX-License-Identifier: GPL-2.0
// Private definitions for the command-objects

#ifndef COMMAND_PRIVATE_H
#define COMMAND_PRIVATE_H

#include "core/dive.h"

#include <vector>
#include <utility>
#include <QVector>

namespace Command {

// Reset the selection to the dives of the "selection" vector and send the appropriate signals.
// Set the current dive to "currentDive". "currentDive" must be an element of "selection" (or
// null if "seletion" is empty). Return true if the selection or current dive changed.
void setSelection(const std::vector<dive *> &selection, dive *currentDive);

// Get currently selectd dives
std::vector<dive *> getDiveSelection();

} // namespace Command

#endif // COMMAND_PRIVATE_H
