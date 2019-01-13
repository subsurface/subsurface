// SPDX-License-Identifier: GPL-2.0
#ifndef COMMAND_H
#define COMMAND_H

#include "core/dive.h"
#include <QVector>
#include <QAction>

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// 1) General commands

void clear();				// Reset the undo stack. Delete all commands.
QAction *undoAction(QObject *parent);	// Create an undo action.
QAction *redoAction(QObject *parent);	// Create an redo action.

// 2) Dive-list related commands

void addDive(dive *d, bool autogroup, bool newNumber); // If d->dive_trip is null and autogroup is true, dives within the auto-group
						       // distance are added to a trip. dive d is consumed (the structure is reset)!
						       // If newNumber is true, the dive is assigned a new number, depending on the
						       // insertion position.
void importDives(struct dive_table *dives, struct trip_table *trips, int flags, const QString &source);
void deleteDive(const QVector<struct dive*> &divesToDelete);
void shiftTime(const QVector<dive *> &changedDives, int amount);
void renumberDives(const QVector<QPair<dive *, int>> &divesToRenumber);
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
