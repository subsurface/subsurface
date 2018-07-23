// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_DIVELIST_H
#define COMMAND_DIVELIST_H

#include "command_base.h"

#include <QVector>

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// This helper structure describes a dive that we want to add.
// Potentially it also adds a trip (if deletion of the dive resulted in deletion of the trip)
struct DiveToAdd {
	OwningDivePtr	 dive;		// Dive to add
	OwningTripPtr	 tripToAdd;	// Not-null if we also have to add a dive
	dive_trip	*trip;		// Trip the dive belongs to, may be null
	int		 idx;		// Position in divelist
};

// This helper structure describes a dive that should be moved to / removed from
// a trip. If the "trip" member is null, the dive is removed from its trip (if
// it is in a trip, that is)
struct DiveToTrip
{
	struct dive	*dive;
	dive_trip	*trip;
};

// This helper structure describes a number of dives to add to /remove from /
// move between trips.
// It has ownership of the trips (if any) that have to be added before hand.
struct DivesToTrip
{
	std::vector<DiveToTrip> divesToMove;		// If dive_trip is null, remove from trip
	std::vector<OwningTripPtr> tripsToAdd;
};

class AddDive : public Base {
public:
	AddDive(dive *dive);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo
	DiveToAdd	diveToAdd;

	// For undo
	dive		*diveToRemove;
};

class DeleteDive : public Base {
public:
	DeleteDive(const QVector<dive *> &divesToDelete);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo
	std::vector<struct dive*> divesToDelete;

	std::vector<OwningTripPtr> tripsToAdd;
	std::vector<DiveToAdd> divesToAdd;
};

class ShiftTime : public Base {
public:
	ShiftTime(const QVector<dive *> &changedDives, int amount);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo and undo
	QVector<dive *> diveList;
	int timeChanged;
};

class RenumberDives : public Base {
public:
	RenumberDives(const QVector<QPair<int, int>> &divesToRenumber);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo and undo: pairs of dive-id / new number
	QVector<QPair<int, int>> divesToRenumber;
};

// The classes RemoveDivesFromTrip, RemoveAutogenTrips, CreateTrip, AutogroupDives
// and MergeTrips all do the same thing, just the intialization differs.
// Therefore, define a base class with the proper data-structures, redo()
// and undo() functions and derive to specialize the initialization.
class TripBase : public Base {
protected:
	void undo() override;
	void redo() override;
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

class SplitDives : public Base {
public:
	// If time is < 0, split at first surface interval
	SplitDives(dive *d, duration_t time);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo
	// For each dive to split, we remove one from and put two dives into the backend
	dive		*diveToSplit;
	DiveToAdd	 splitDives[2];

	// For undo
	// For each dive to unsplit, we remove two dives from and add one into the backend
	DiveToAdd	 unsplitDive;
	dive		*divesToUnsplit[2];
};

class MergeDives : public Base {
public:
	MergeDives(const QVector<dive *> &dives);
private:
	void undo() override;
	void redo() override;
	bool workToBeDone() override;

	// For redo
	// Add one and remove a batch of dives
	DiveToAdd		 mergedDive;
	std::vector<dive *>	 divesToMerge;

	// For undo
	// Remove one and add a batch of dives
	dive			*diveToUnmerge;
	std::vector<DiveToAdd>	 unmergedDives;

	// For undo and redo
	QVector<QPair<int, int>> divesToRenumber;
};

} // namespace Command

#endif // COMMAND_DIVELIST_H
