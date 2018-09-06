// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETRIPMODEL_H
#define DIVETRIPMODEL_H

#include "treemodel.h"
#include "core/dive.h"
#include <string>

struct DiveItem : public TreeItem {
	Q_DECLARE_TR_FUNCTIONS(TripItem)
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

	QVariant data(int column, int role) const override;
	dive *d;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
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

struct TripItem : public TreeItem {
	Q_DECLARE_TR_FUNCTIONS(TripItem)
public:
	QVariant data(int column, int role) const override;
	dive_trip_t *trip;
};

class DiveTripModel : public TreeModel {
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

private:
	void setupModelData();
	Layout currentLayout;
};

#endif
