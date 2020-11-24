// SPDX-License-Identifier: GPL-2.0

// The DiveListNotifier emits signals when the dive-list changes (dives/trips/divesites created/deleted/moved/edited)

#ifndef DIVELISTNOTIFIER_H
#define DIVELISTNOTIFIER_H

#include "core/dive.h"
#include "core/pictureobj.h"

#include <QObject>

struct device;

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
	unsigned int wavesize : 1;
	unsigned int current : 1;
	unsigned int surge : 1;
	unsigned int chill : 1;
	unsigned int suit : 1;
	unsigned int tags : 1;
	unsigned int mode : 1;
	unsigned int notes : 1;
	unsigned int salinity : 1;
	unsigned int invalid : 1;
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
		WAVESIZE = 1 << 12,
		CURRENT = 1 << 13,
		SURGE = 1 << 14,
		CHILL = 1 << 15,
		SUIT = 1 << 16,
		TAGS = 1 << 17,
		MODE = 1 << 18,
		NOTES = 1 << 19,
		SALINITY = 1 << 20,
		INVALID = 1 << 21
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
	// The core structures were completely reset. Repopulate all models.
	void dataReset();

	// The settings changed. Repopulate / rerender unit-dependent data, etc.
	void settingsChanged();

	// Note that there are no signals for trips being added and created
	// because these events never happen without a dive being added, removed or moved.
	// The dives are always sorted according to the dives_less_than() function of the core.
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void divesTimeChanged(timestamp_t delta, const QVector<dive *> &dives);
	void divesImported(); // A general signal when multiple dives have been imported.

	void cylindersReset(const QVector<dive *> &dives);
	void cylinderAdded(dive *d, int pos);
	void cylinderRemoved(dive *d, int pos);
	void cylinderEdited(dive *d, int pos);
	void weightsystemsReset(const QVector<dive *> &dives);
	void weightAdded(dive *d, int pos);
	void weightRemoved(dive *d, int pos);
	void weightEdited(dive *d, int pos);

	// Trip edited signal
	void tripChanged(dive_trip *trip, TripField field);

	// Selection changes
	void divesSelected(const QVector<dive *> &dives);

	// Dive site signals. Add and delete events are sent per dive site and
	// provide an index into the global dive site table.
	void diveSiteAdded(dive_site *ds, int idx);
	void diveSiteDeleted(dive_site *ds, int idx);
	void diveSiteDiveCountChanged(dive_site *ds);
	void diveSiteChanged(dive_site *ds, int field); // field according to LocationInformationModel
	void diveSiteDivesChanged(dive_site *ds); // The dives associated with that site changed

	// Filter-related signals
	void numShownChanged();
	void filterReset();

	// Event-related signals. Currently, we're very blunt: only one signal for any changes to the events.
	void eventsChanged(dive *d);

	// Picture (media) related signals
	void pictureOffsetChanged(dive *d, QString filename, offset_t offset);
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<PictureObj> pics);

	// Devices related signals
	void deviceAdded(int index);
	void deviceDeleted(int index);
	void deviceEdited(int index);

	// Filter related signals
	void filterPresetAdded(int index);
	void filterPresetRemoved(int index);
	void filterPresetChanged(int index);

	// This signal is emited every time a command is executed.
	// This is used to hide an old multi-dives-edited warning message.
	// This is necessary, so that the user can't click on the "undo" button and undo
	// an unrelated command.
	void commandExecuted();
};

// The DiveListNotifier class has only trivial state.
// We can simply define it as a global object.
extern DiveListNotifier diveListNotifier;

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
	wavesize((flags & WAVESIZE) != 0),
	current((flags & CURRENT) != 0),
	surge((flags & SURGE) != 0),
	chill((flags & CHILL) != 0),
	suit((flags & SUIT) != 0),
	tags((flags & TAGS) != 0),
	mode((flags & MODE) != 0),
	notes((flags & NOTES) != 0),
	salinity((flags & SALINITY) != 0),
	invalid((flags & INVALID) != 0)
{
}

inline TripField::TripField(int flags) :
	location((flags & LOCATION) != 0),
	notes((flags & NOTES) != 0)
{
}
#endif
