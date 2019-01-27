// SPDX-License-Identifier: GPL-2.0

// The DiveListNotifier emits signals when the dive-list changes (dives/trips created/deleted/moved/edited)
// Note that vectors are passed by reference, so this will only work for signals inside the UI thread!

#ifndef DIVELISTNOTIFIER_H
#define DIVELISTNOTIFIER_H

#include "core/dive.h"

#include <QObject>

// Dive fields that can be edited.
// Use "enum class" to not polute the global name space.
enum class DiveField {
	DATETIME,
	AIR_TEMP,
	WATER_TEMP,
	LOCATION,
	DIVEMASTER,
	BUDDY,
	RATING,
	VISIBILITY,
	SUIT,
	TAGS,
	MODE,
	NOTES,
};

class DiveListNotifier : public QObject {
	Q_OBJECT
signals:

	// Note that there are no signals for trips being added / created / time-shifted,
	// because these events never happen without a dive being added / created / time-shifted.

	// We send one divesAdded, divesDeleted, divesChanged and divesTimeChanged, divesSelected
	// signal per trip (non-associated dives being considered part of the null trip). This is
	// ideal for the tree-view, but might be not-so-perfect for the list view, if trips intermingle
	// or the deletion spans multiple trips. But most of the time only dives of a single trip
	// will be affected and trips don't overlap, so these considerations are moot.
	// Notes:
	// - The dives are always sorted by start-time.
	// - The "trip" arguments are null for top-level-dives.
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void divesChanged(dive_trip *trip, const QVector<dive *> &dives);
	void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void divesTimeChanged(dive_trip *trip, timestamp_t delta, const QVector<dive *> &dives);

	// Selection-signals come in two kinds:
	//  - divesSelected, divesDeselected and currentDiveChanged are finer grained and are
	//    called batch-wise per trip (except currentDiveChanged, of course). These signals
	//    are used by the dive-list model and view to correctly highlight the correct dives.
	//  - selectionChanged() is called once at the end of commands if either the selection
	//    or the current dive changed. It is used by the main-window / profile to update
	//    their data.
	void divesSelected(dive_trip *trip, const QVector<dive *> &dives);
	void divesDeselected(dive_trip *trip, const QVector<dive *> &dives);
	void currentDiveChanged();
	void selectionChanged();

	// Signals emitted when dives are edited.
	void divesEdited(const QVector<dive *> &dives, DiveField);
public:
	// Desktop uses the QTreeView class to present the list of dives. The layout
	// of this class gives us a very fundamental problem, as we can not easily
	// distinguish between user-initiated changes of the selection and changes
	// that are due to actions of the Command-classes. To solve this problem,
	// the frontend can use this function to query whether a dive list-modifying
	// command is currently executed. If this function returns true, the
	// frontend is supposed to not modify the selection.
	bool inCommand() const;

	// The following class and function are used by divelist-modifying commands
	// to signal that they are in-flight. If the returned object goes out of scope,
	// the command-in-flight status is reset to its previous value. Thus, the
	// function can be called recursively.
	class InCommandMarker {
		DiveListNotifier &notifier;
		bool oldValue;
		InCommandMarker(DiveListNotifier &);
		friend DiveListNotifier;
	public:
		~InCommandMarker();
	};

	// Usage:
	// void doWork()
	// {
	// 	auto marker = diveListNotifier.enterCommand();
	// 	... do work ...
	// }
	InCommandMarker enterCommand();
private:
	friend InCommandMarker;
	bool commandExecuting;
};

// The DiveListNotifier class has only trivial state.
// We can simply define it as a global object.
extern DiveListNotifier diveListNotifier;

// InCommandMarker is so trivial that the functions can be inlined.
// TODO: perhaps move this into own header-file.
inline DiveListNotifier::InCommandMarker::InCommandMarker(DiveListNotifier &notifierIn) : notifier(notifierIn),
	oldValue(notifier.commandExecuting)
{
	notifier.commandExecuting = true;
}

inline DiveListNotifier::InCommandMarker::~InCommandMarker()
{
	notifier.commandExecuting = oldValue;
}

inline bool DiveListNotifier::inCommand() const
{
	return commandExecuting;
}

inline DiveListNotifier::InCommandMarker DiveListNotifier::enterCommand()
{
	return InCommandMarker(*this);
}

#endif
