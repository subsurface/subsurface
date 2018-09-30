// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETRIPMODEL_H
#define DIVETRIPMODEL_H

#include "core/dive.h"
#include <QAbstractItemModel>
#include <QCoreApplication>		// For Q_DECLARE_TR_FUNCTIONS

struct DiveItem {
	Q_DECLARE_TR_FUNCTIONS(TripItem)		// Is that TripItem on purpose?
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

	QVariant data(int column, int role) const;
	dive *d;
	DiveItem(dive *dIn) : d(dIn) {}			// Trivial constructor
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	QString displayDate() const;
	QString displayDuration() const;
	QString displayDepth() const;
	QString displayDepthWithUnit() const;
	QString displayTemperature() const;
	QString displayTemperatureWithUnit() const;
	QString displayWeight() const;
	QString displayWeightWithUnit() const;
	QString displaySac() const;
	QString displaySacWithUnit() const;
	QString displayTags() const;
	int countPhotos() const;
	int weight() const;
};

struct TripItem {
	Q_DECLARE_TR_FUNCTIONS(TripItem)
public:
	QVariant data(int column, int role) const;
	dive_trip_t *trip;
	TripItem(dive_trip_t *tIn) : trip(tIn) {}	// Trivial constructor
};

class DiveTripModel : public QAbstractItemModel {
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
		SORT_ROLE,
		DIVE_IDX
	};
	enum Layout {
		TREE,
		LIST,
		CURRENT
	};

	static DiveTripModel *instance();
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	DiveTripModel(QObject *parent = 0);
	Layout layout() const;
	void setLayout(Layout layout);
	QVariant data(const QModelIndex &index, int role) const;
	int columnCount(const QModelIndex&) const;
	int rowCount(const QModelIndex &parent) const;
	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &index) const;

private:
	// The model has up to two levels. At the top level, we have either trips or dives
	// that do not belong to trips. Such a top-level item is represented by the "Item"
	// struct. Two cases two consider:
	// 1) If "trip" is non-null, then this is a dive-trip and the dives are collected
	// in the dives vector.  Note that in principle we could also get the dives in a
	// trip from the backend, but there they are collected in a linked-list, which is
	// quite inconvenient to access.
	// 2) If "trip" is null, this is a dive and dives is supposed to contain exactly
	// one element, which is the corresponding dive.
	struct Item {
		dive_trip		*trip;
		QVector<dive *>		 dives;
		Item(dive_trip *t, dive *d);	// Initialize a trip with one dive
		Item(dive *d);			// Initialize a top-level dive
	};

	dive *diveOrNull(const QModelIndex &index) const;	// Returns a dive if this index represents a dive, null otherwise
	QPair<dive_trip *, dive *> tripOrDive(const QModelIndex &index) const;
								// Returns either a pointer to a trip or a dive, or twice null of index is invalid
								// null, something is really wrong
	void setupModelData();
	std::vector<Item> items;				// Use std::vector for convenience of emplace_back()
	Layout currentLayout;
};

#endif
