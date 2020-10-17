// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_DIVELIST_H
#define COMMAND_DIVELIST_H

#include "command_base.h"
#include "core/filterpreset.h"
#include "core/device.h"

#include <QVector>

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// This helper structure describes a dive that we want to add.
struct DiveToAdd {
	OwningDivePtr	 dive;		// Dive to add
	dive_trip	*trip;		// Trip the dive belongs to, may be null
	dive_site	*site;		// Site the dive is associated with, may be null
};

// Multiple trips, dives and dive sites that have to be added for a command
struct DivesAndTripsToAdd {
	std::vector<DiveToAdd> dives;
	std::vector<OwningTripPtr> trips;
	std::vector<OwningDiveSitePtr> sites;
};

// Dives and sites that have to be removed for a command
struct DivesAndSitesToRemove {
	std::vector<dive *> dives;
	std::vector<dive_site *> sites;
};

// This helper structure describes a dive that should be moved to / removed from
// a trip. If the "trip" member is null, the dive is removed from its trip (if
// it is in a trip, that is)
struct DiveToTrip
{
	struct dive	*dive;
	dive_trip	*trip;
};

// This helper structure describes a number of dives to add to / remove from /
// move between trips.
// It has ownership of the trips (if any) that have to be added before hand.
struct DivesToTrip
{
	std::vector<DiveToTrip> divesToMove;		// If dive_trip is null, remove from trip
	std::vector<OwningTripPtr> tripsToAdd;
};

// All divelist commands derive from a common base class. It keeps track
// of dive site counts that may have changed.
// Derived classes implement the virtual methods redoit() and undoit()
// [Yes, the names could be more expressive].
class DiveListBase : public Base {
protected:
	// These are helper functions to add / remove dive from the C-core structures.
	DiveToAdd removeDive(struct dive *d, std::vector<OwningTripPtr> &tripsToAdd);
	dive *addDive(DiveToAdd &d);
	DivesAndTripsToAdd removeDives(DivesAndSitesToRemove &divesAndSitesToDelete);
	DivesAndSitesToRemove addDives(DivesAndTripsToAdd &toAdd);

	// Register dive sites where counts changed so that we can signal the frontend later.
	void diveSiteCountChanged(struct dive_site *ds);

private:
	// Keep track of dive sites where the number of dives changed
	std::vector<dive_site *> sitesCountChanged;
	void initWork();
	void finishWork(); // update dive site counts
	void undo() override;
	void redo() override;
	virtual void redoit() = 0;
	virtual void undoit() = 0;
};

class AddDive : public DiveListBase {
public:
	AddDive(dive *dive, bool autogroup, bool newNumber);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	// Note: we a multi-dive structure even though we add only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	DivesAndTripsToAdd	divesToAdd;

	// For undo
	DivesAndSitesToRemove	divesAndSitesToRemove;
	std::vector<dive *>	selection;
	dive *			currentDive;
};

class ImportDives : public DiveListBase {
public:
	// Note: dives and trips are consumed - after the call they will be empty.
	ImportDives(struct dive_table *dives, struct trip_table *trips, struct dive_site_table *sites,
		    struct device_table *devices, struct filter_preset_table *filter_presets, int flags,
		    const QString &source);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo and undo
	DivesAndTripsToAdd	divesToAdd;
	DivesAndSitesToRemove	divesAndSitesToRemove;
	struct device_table	devicesToAddAndRemove;

	// For redo
	std::vector<OwningDiveSitePtr>	sitesToAdd;
	std::vector<std::pair<QString,FilterData>>
					filterPresetsToAdd;

	// For undo
	std::vector<dive_site *>	sitesToRemove;
	std::vector<dive *>		selection;
	dive				*currentDive;
	std::vector<int>		filterPresetsToRemove;
};

class DeleteDive : public DiveListBase {
public:
	DeleteDive(const QVector<dive *> &divesToDelete);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	DivesAndSitesToRemove divesToDelete;

	std::vector<OwningTripPtr> tripsToAdd;
	DivesAndTripsToAdd divesToAdd;
};

class ShiftTime : public DiveListBase {
public:
	ShiftTime(std::vector<dive *> changedDives, int amount);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo and undo
	std::vector<dive *> diveList;
	int timeChanged;
};

class RenumberDives : public DiveListBase {
public:
	RenumberDives(const QVector<QPair<dive *, int>> &divesToRenumber);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo and undo: pairs of dive-id / new number
	QVector<QPair<dive *, int>> divesToRenumber;
};

// The classes RemoveDivesFromTrip, RemoveAutogenTrips, CreateTrip, AutogroupDives
// and MergeTrips all do the same thing, just the intialization differs.
// Therefore, define a base class with the proper data-structures, redo()
// and undo() functions and derive to specialize the initialization.
class TripBase : public DiveListBase {
protected:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo and undo
	DivesToTrip divesToMove;
};
struct RemoveDivesFromTrip : public TripBase {
	RemoveDivesFromTrip(const QVector<dive *> &divesToRemove);
};
struct RemoveAutogenTrips : public TripBase {
	RemoveAutogenTrips();
};
struct AddDivesToTrip : public TripBase {
	AddDivesToTrip(const QVector<dive *> &divesToAdd, dive_trip *trip);
};
struct CreateTrip : public TripBase {
	CreateTrip(const QVector<dive *> &divesToAdd);
};
struct AutogroupDives : public TripBase {
	AutogroupDives();
};
struct MergeTrips : public TripBase {
	MergeTrips(dive_trip *trip1, dive_trip *trip2);
};

class SplitDivesBase : public DiveListBase {
protected:
	SplitDivesBase(dive *old, std::array<dive *, 2> newDives);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	// For each dive to split, we remove one from and put two dives into the backend
	// Note: we use a vector even though we split only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	DivesAndSitesToRemove	diveToSplit;
	DivesAndTripsToAdd	splitDives;

	// For undo
	// For each dive to unsplit, we remove two dives from and add one into the backend
	// Note: we use a multi-dive structure even though we unsplit only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	DivesAndTripsToAdd	unsplitDive;
	DivesAndSitesToRemove	divesToUnsplit;
};

class SplitDives : public SplitDivesBase {
public:
	// If time is < 0, split at first surface interval
	SplitDives(dive *d, duration_t time);
};

class SplitDiveComputer : public SplitDivesBase {
public:
	// If time is < 0, split at first surface interval
	SplitDiveComputer(dive *d, int dc_num);
};

// When manipulating dive computers (moving, deleting) we go the ineffective,
// but simple and robust way: We keep two full copies of the dive (before and after).
// Removing and readding assures that the dive stays at the correct
// position in the list (the dive computer list is used for sorting dives).
class DiveComputerBase : public DiveListBase {
protected:
	// old_dive must be a dive known to the core.
	// new_dive must be new dive whose ownership is taken.
	DiveComputerBase(dive *old_dive, dive *new_dive, int dc_nr_after);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

protected:
	// For redo and undo
	DivesAndTripsToAdd	diveToAdd;
	DivesAndSitesToRemove	diveToRemove;
	int			dc_nr_before, dc_nr_after;
};

class MoveDiveComputerToFront : public DiveComputerBase {
public:
	MoveDiveComputerToFront(dive *d, int dc_num);
};

class DeleteDiveComputer : public DiveComputerBase {
public:
	DeleteDiveComputer(dive *d, int dc_num);
};

class MergeDives : public DiveListBase {
public:
	MergeDives(const QVector<dive *> &dives);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	// Add one and remove a batch of dives
	// Note: we use a multi-dives structure even though we add only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	DivesAndTripsToAdd	mergedDive;
	DivesAndSitesToRemove	divesToMerge;

	// For undo
	// Remove one and add a batch of dives
	// Note: we use a vector even though we remove only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	DivesAndSitesToRemove	diveToUnmerge;
	DivesAndTripsToAdd	unmergedDives;

	// For undo and redo
	QVector<QPair<dive *, int>> divesToRenumber;
};

} // namespace Command

#endif // COMMAND_DIVELIST_H
