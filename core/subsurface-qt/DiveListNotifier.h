// SPDX-License-Identifier: GPL-2.0

// The DiveListNotifier emits signals when the dive-list changes (dives/trips/divesites created/deleted/moved/edited)

#ifndef DIVELISTNOTIFIER_H
#define DIVELISTNOTIFIER_H

#include "core/dive.h"

#include <QObject>

// Dive and trip fields that can be edited. Use bit fields so that we can pass multiple fields at once.
// Provides an inlined flag-based constructur because sadly C-style designated initializers are only supported since C++20.
struct DiveField {
	// Note: using int instead of the more natural bool, because gcc produces significantly worse code with
	// bool. clang, on the other hand, does fine.
	unsigned int nr : 1;
	unsigned int datetime : 1;
	unsigned int depth : 1;
	unsigned int duration : 1;
	unsigned int air_temp : 1;
	unsigned int water_temp : 1;
	unsigned int atm_press : 1;
	unsigned int divesite : 1;
	unsigned int divemaster : 1;
	unsigned int buddy : 1;
	unsigned int rating : 1;
	unsigned int visibility : 1;
	unsigned int suit : 1;
	unsigned int tags : 1;
	unsigned int mode : 1;
	unsigned int notes : 1;
	unsigned int salinity : 1;
	enum Flags {
		NONE = 0,
		NR = 1 << 0,
		DATETIME = 1 << 1,
		DEPTH = 1 << 2,
		DURATION = 1 << 3,
		AIR_TEMP = 1 << 4,
		WATER_TEMP = 1 << 5,
		ATM_PRESS = 1 << 6,
		DIVESITE = 1 << 7,
		DIVEMASTER = 1 << 8,
		BUDDY = 1 << 9,
		RATING = 1 << 10,
		VISIBILITY = 1 << 11,
		SUIT = 1 << 12,
		TAGS = 1 << 13,
		MODE = 1 << 14,
		NOTES = 1 << 15,
		SALINITY = 1 << 16
	};
	DiveField(int flags);
};
struct TripField {
	unsigned int location : 1;
	unsigned int notes : 1;
	enum Flags {
		NONE = 0,
		LOCATION = 1 << 0,
		NOTES = 1 << 1
	};
	TripField(int flags);
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

inline DiveField::DiveField(int flags) :
	nr((flags & NR) != 0),
	datetime((flags & DATETIME) != 0),
	depth((flags & DEPTH) != 0),
	duration((flags & DURATION) != 0),
	air_temp((flags & AIR_TEMP) != 0),
	water_temp((flags & WATER_TEMP) != 0),
	atm_press((flags & ATM_PRESS) != 0),
	divesite((flags & DIVESITE) != 0),
	divemaster((flags & DIVEMASTER) != 0),
	buddy((flags & BUDDY) != 0),
	rating((flags & RATING) != 0),
	visibility((flags & VISIBILITY) != 0),
	suit((flags & SUIT) != 0),
	tags((flags & TAGS) != 0),
	mode((flags & MODE) != 0),
	notes((flags & NOTES) != 0),
	salinity((flags & SALINITY) != 0)
{
}

inline TripField::TripField(int flags) :
	location((flags & LOCATION) != 0),
	notes((flags & NOTES) != 0)
{
}
#endif
