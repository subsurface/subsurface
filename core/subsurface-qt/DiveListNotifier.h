// SPDX-License-Identifier: GPL-2.0

// The DiveListNotifier emits signals when the dive-list changes (dives/trips created/deleted/moved)
// Note that vectors are passed by reference, so this will only work for signals inside the UI thread!

#ifndef DIVELISTNOTIFIER_H
#define DIVELISTNOTIFIER_H

#include "core/dive.h"

#include <QObject>

class DiveListNotifier : public QObject {
	Q_OBJECT
signals:

	// Note that there are no signals for trips being added / created / time-shifted,
	// because these events never happen without a dive being added / created / time-shifted.

	// We send one divesAdded, divesDeleted, divesChanged and divesTimeChanged signal per trip
	// (non-associated dives being considered part of the null trip). This is ideal for the
	// tree-view, but might be not-so-perfect for the list view, if trips intermingle or
	// the deletion spans multiple trips. But most of the time only dives of a single trip
	// will be affected and trips don't overlap, so these considerations are moot.
	// Notes:
	// - The dives are always sorted by start-time.
	// - The "trip" arguments are null for top-level-dives.
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void divesChanged(dive_trip *trip, const QVector<dive *> &dives);
	void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void divesTimeChanged(dive_trip *trip, timestamp_t delta, const QVector<dive *> &dives);
};

// The DiveListNotifier class has no state and no constructor.
// We can simply define it as a global object.
extern DiveListNotifier diveListNotifier;

#endif
