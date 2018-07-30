// SPDX-License-Identifier: GPL-2.0
#ifndef COMMAND_H
#define COMMAND_H

#include "core/dive.h"
#include <QVector>
#include <QAction>

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// General commands
void clear();				// Reset the undo stack. Delete all commands.
QAction *undoAction(QObject *parent);	// Create an undo action.
QAction *redoAction(QObject *parent);	// Create an redo action.

// Dive-list related commands
void addDive(dive *d, bool autogroup);
void deleteDive(const QVector<struct dive*> &divesToDelete);
void shiftTime(const QVector<dive *> &changedDives, int amount);
void renumberDives(const QVector<QPair<int, int>> &divesToRenumber);
void removeDivesFromTrip(const QVector<dive *> &divesToRemove);
void removeAutogenTrips();
void addDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip);
void createTrip(const QVector<dive *> &divesToAddIn);
void autogroupDives();
void mergeTrips(dive_trip *trip1, dive_trip *trip2);
void splitDives(dive *d, duration_t time);
void mergeDives(const QVector <dive *> &dives);

} // namespace Command

#endif // COMMAND_H
