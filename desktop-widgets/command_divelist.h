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

// All divelist commands derive from a common base class, which has a flag
// for when then selection changed. In such a case, in the redo() and undo()
// methods a signal will be sent. The base-class implements redo() and undo(),
// which resets the flag and sends a signal. Derived classes implement the
// virtual methods redoit() and undoit() [Yes, the names could be more expressive].
// Moreover, the class implements helper methods, which set the selectionChanged
// flag accordingly.
class DiveListBase : public Base {
protected:
	DiveListBase();

	// These are helper functions to add / remove dive from the C-core structures,
	// which set the selectionChanged flag if the added / removed dive was selected.
	DiveToAdd removeDive(struct dive *d);
	dive *addDive(DiveToAdd &d);
	std::vector<DiveToAdd> removeDives(std::vector<dive *> &divesToDelete);
	std::vector<dive *> addDives(std::vector<DiveToAdd> &divesToAdd);

	// Set the selection to a given state. Set the selectionChanged flag if anything changed.
	void restoreSelection(const std::vector<dive *> &selection, dive *currentDive);

	// On first execution, the selections before and after execution will
	// be remembered. On all further executions, the selection will be reset to
	// the remembered values.
	// Note: Therefore, commands should manipulate the selection and send the
	// corresponding signals only on first execution!
	bool firstExecution;

	// Commands set this flag if the selection changed on first execution.
	// Only then, a new the divelist will be scanned again after the command.
	// If this flag is set on first execution, a selectionChanged signal will
	// be sent.
	bool selectionChanged;
private:
	void initWork(); // reset selectionChanged flag
	void finishWork(); // emit signal if selection changed
	void undo() override;
	void redo() override;
	virtual void redoit() = 0;
	virtual void undoit() = 0;
};

class AddDive : public DiveListBase {
public:
	AddDive(dive *dive, bool autogroup);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	// Note: we use a vector even though we add only a single dive, so
	// that we can reuse the multi-dive functions of the other commands.
	std::vector<DiveToAdd>	divesToAdd;

	// For undo
	std::vector<dive *>	divesToRemove;
	std::vector<dive *>	selection;
	dive *			currentDive;
};

class DeleteDive : public DiveListBase {
public:
	DeleteDive(const QVector<dive *> &divesToDelete);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo
	std::vector<struct dive*> divesToDelete;

	std::vector<OwningTripPtr> tripsToAdd;
	std::vector<DiveToAdd> divesToAdd;
};

class ShiftTime : public DiveListBase {
public:
	ShiftTime(const QVector<dive *> &changedDives, int amount);
private:
	void undoit() override;
	void redoit() override;
	bool workToBeDone() override;

	// For redo and undo
	QVector<dive *> diveList;
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

class SplitDives : public DiveListBase {
public:
	// If time is < 0, split at first surface interval
	SplitDives(dive *d, duration_t time);
private:
	void undoit() override;
	void redoit() override;
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

class MergeDives : public DiveListBase {
public:
	MergeDives(const QVector<dive *> &dives);
private:
	void undoit() override;
	void redoit() override;
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
	QVector<QPair<dive *, int>> divesToRenumber;
};

} // namespace Command

#endif // COMMAND_DIVELIST_H
