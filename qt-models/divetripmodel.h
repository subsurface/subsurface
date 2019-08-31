// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETRIPMODEL_H
#define DIVETRIPMODEL_H

#include "core/dive.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include <QAbstractItemModel>

// There are two different representations of the dive list:
// 1) Tree view: two-level model where dives are grouped by trips
// 2) List view: one-level model where dives are sorted by one out
//    of many keys (e.g. date, depth, etc.).
//
// These two representations are realized by two classe, viz.
// DiveTripModelTree and DiveTripModelList. Both classes derive
// from DiveTripModelBase, which implements common features (e.g.
// definition of the column types, access of data from the core
// structures) and a common interface.
//
// The currently active model is set via DiveTripModelBase::resetModel().
// This will create a new model. The model can be accessed with
// DiveTripModelBase::instance(). Any pointer obtained by instance()
// is invalid after a call to resetModel()! Yes, this is surprising
// behavior, so care must be taken.
class DiveTripModelBase : public QAbstractItemModel {
	Q_OBJECT
public:
	enum Column {
		NR,
		DATE,
		RATING,
		DEPTH,
		DURATION,
		TEMPERATURE,
		TOTALWEIGHT,
		SUIT,
		CYLINDER,
		GAS,
		SAC,
		OTU,
		MAXCNS,
		TAGS,
		PHOTOS,
		BUDDIES,
		COUNTRY,
		LOCATION,
		COLUMNS
	};

	enum ExtraRoles {
		STAR_ROLE = Qt::UserRole + 1,
		DIVE_ROLE,
		TRIP_ROLE,
		DIVE_IDX,
		SHOWN_ROLE,
		SELECTED_ROLE
	};
	enum Layout {
		TREE,
		LIST,
	};

	// Functions implemented by base class
	static DiveTripModelBase *instance();

	// Reset the model using the given layout. After this call instance() will return
	// a newly allocated object and the old model will have been destroyed! Thus, the
	// caller is repsonsible of removing all references to any previous model obtained
	// by insance().
	static void resetModel(Layout layout);

	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	DiveTripModelBase(QObject *parent = 0);
	int columnCount(const QModelIndex&) const;
	virtual void filterFinished() = 0;
	virtual bool setShown(const QModelIndex &idx, bool shown) = 0;

	// Used for sorting. This is a bit of a layering violation, as sorting should be performed
	// by the higher-up QSortFilterProxyModel, but it makes things so much easier!
	virtual bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const = 0;

signals:
	// The propagation of selection changes is complex.
	// The control flow of dive-selection goes:
	// Commands/DiveListNotifier ---(dive */dive_trip *)---> DiveTripModel ---(QModelIndex)---> DiveListView
	// i.e. The command objects send changes in terms of pointer-to-dives, which the DiveTripModel transforms
	// into QModelIndexes according to the current view (tree/list). Finally, the DiveListView transforms these
	// indexes into local indexes according to current sorting/filtering and instructs the QSelectionModel to
	// perform the appropriate actions.
	void selectionChanged(const QVector<QModelIndex> &indexes);
	void newCurrentDive(QModelIndex index);
protected:
	// Access trip and dive data
	static QVariant diveData(const struct dive *d, int column, int role);
	static QVariant tripData(const dive_trip *trip, int column, int role);

	virtual dive *diveOrNull(const QModelIndex &index) const = 0;	// Returns a dive if this index represents a dive, null otherwise
};

class DiveTripModelTree : public DiveTripModelBase
{
	Q_OBJECT
public slots:
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void diveSiteChanged(dive_site *ds, int field);
	void divesChanged(const QVector<dive *> &dives);
	void divesTimeChanged(timestamp_t delta, const QVector<dive *> &dives);
	void divesSelected(const QVector<dive *> &dives, dive *current);
	void tripChanged(dive_trip *trip, TripField);

public:
	DiveTripModelTree(QObject *parent = nullptr);
private:
	int rowCount(const QModelIndex &parent) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	void filterFinished() override;
	bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const override;
	void divesSelectedTrip(dive_trip *trip, const QVector<dive *> &dives, QVector<QModelIndex> &);
	dive *diveOrNull(const QModelIndex &index) const override;
	bool setShown(const QModelIndex &idx, bool shown);
	void divesChangedTrip(dive_trip *trip, const QVector<dive *> &dives);
	void divesTimeChangedTrip(dive_trip *trip, timestamp_t delta, const QVector<dive *> &dives);

	// The tree model has two levels. At the top level, we have either trips or dives
	// that do not belong to trips. Such a top-level item is represented by the "Item"
	// struct, which is based on the dive_or_trip structure.
	// If it is a trip, additionally, the dives are collected in a vector.
	// The items are ordered chronologically according to the dive_or_trip_less_than()
	// function, which guarantees a stable ordering even in the case of equal timestamps.
	// For dives and trips, it place dives chronologically after trips, so that in
	// the default-descending view they are shown before trips.
	struct Item {
		dive_or_trip		d_or_t;
		std::vector<dive *>	dives;			// std::vector<> instead of QVector for insert() with three iterators
		bool			shown;
		Item(dive_trip *t, const QVector<dive *> &dives);
		Item(dive_trip *t, dive *d);			// Initialize a trip with one dive
		Item(dive *d);					// Initialize a top-level dive
		bool isDive(const dive *) const;		// Helper function: is this the give dive?
		dive *getDive() const;				// Helper function: returns top-level-dive or null
		timestamp_t when() const;			// Helper function: start time of dive *or* trip
	};
	std::vector<Item> items;				// Use std::vector for convenience of emplace_back()

	dive_or_trip tripOrDive(const QModelIndex &index) const;
								// Returns either a pointer to a trip or a dive, or twice null of index is invalid
								// null, something is really wrong
	// Addition and deletion of dives
	void addDivesToTrip(int idx, const QVector<dive *> &dives);
	void topLevelChanged(int idx);

	// Access trips and dives
	int findTripIdx(const dive_trip *trip) const;
	int findDiveIdx(const dive *d) const;			// Find _top_level_ dive
	int findDiveInTrip(int tripIdx, const dive *d) const;	// Find dive inside trip. Second parameter is index of trip
	int findInsertionIndex(const dive_trip *trip) const;	// Where to insert trip

	// Comparison function between dive and arbitrary entry
	static bool dive_before_entry(const dive *d, const Item &entry);
};

class DiveTripModelList : public DiveTripModelBase
{
	Q_OBJECT
public slots:
	void divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives);
	void divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives);
	void diveSiteChanged(dive_site *ds, int field);
	void divesChanged(const QVector<dive *> &dives);
	void divesTimeChanged(timestamp_t delta, const QVector<dive *> &dives);
	// Does nothing in list view.
	//void divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives);
	void divesSelected(const QVector<dive *> &dives, dive *current);

public:
	DiveTripModelList(QObject *parent = nullptr);
private:
	int rowCount(const QModelIndex &parent) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	void filterFinished() override;
	bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const override;
	dive *diveOrNull(const QModelIndex &index) const override;
	bool setShown(const QModelIndex &idx, bool shown);

	std::vector<dive *> items;				// TODO: access core data directly
};

#endif
