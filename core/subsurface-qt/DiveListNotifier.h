// SPDX-License-Identifier: GPL-2.0

// The DiveListNotifier emits signals when the dive-list changes (dives/trips/divesites created/deleted/moved/edited)

#ifndef DIVELISTNOTIFIER_H
#define DIVELISTNOTIFIER_H

#include "core/dive.h"

#include <QObject>

// Dive and trip fields that can be edited.
// Use "enum class" to not polute the global name space.
enum class DiveField {
	NR,
	DATETIME,
	DEPTH,
	DURATION,
	AIR_TEMP,
	WATER_TEMP,
	ATM_PRESS,
	DIVESITE,
	DIVEMASTER,
	BUDDY,
	RATING,
	VISIBILITY,
	SUIT,
	TAGS,
	MODE,
	NOTES,
};
enum class TripField {
	LOCATION,
	NOTES
};

class DiveListNotifier : public QObject {
	Q_OBJECT
signals:
	// Note that there are no signals for trips being added and created
	// because these events never happen without a dive being added, removed or moved.
	// The dives are always sorted according to the dives_less_than() function of the core.
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void divesTimeChanged(timestamp_t delta, const QVector<dive *> &dives);

	void cylindersReset(const QVector<dive *> &dives);
	void weightsystemsReset(const QVector<dive *> &dives);

	// Trip edited signal
	void tripChanged(dive_trip *trip, TripField field);

	void divesSelected(const QVector<dive *> &dives, dive *current);

	// Dive site signals. Add and delete events are sent per dive site and
	// provide an index into the global dive site table.
	void diveSiteAdded(dive_site *ds, int idx);
	void diveSiteDeleted(dive_site *ds, int idx);
	void diveSiteDiveCountChanged(dive_site *ds);
	void diveSiteChanged(dive_site *ds, int field); // field according to LocationInformationModel
	void diveSiteDivesChanged(dive_site *ds); // The dives associated with that site changed

	// This signal is emited every time a command is executed.
	// This is used to hide an old multi-dives-edited warning message.
	// This is necessary, so that the user can't click on the "undo" button and undo
	// an unrelated command.
	void commandExecuted();
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
