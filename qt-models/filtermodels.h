// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include "divetripmodel.h"

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>

#include <stdint.h>
#include <vector>

struct dive;
struct dive_trip;

struct FilterData {
	// The mode ids are chosen such that they can be directly converted from / to combobox indices.
	enum class Mode {
		ALL_OF = 0,
		ANY_OF = 1,
		NONE_OF = 2
	};

	bool validFilter = false;
	int minVisibility = 0;
	int maxVisibility = 5;
	int minRating = 0;
	int maxRating = 5;
	// The default minimum and maximum temperatures are set such that all
	// physically reasonable dives are shown. Note that these values should
	// work for both Celcius and Fahrenheit scales.
	double minWaterTemp = -10;
	double maxWaterTemp = 200;
	double minAirTemp = -50;
	double maxAirTemp = 200;
	QDateTime fromDate = QDateTime(QDate(1980,1,1));
	QTime fromTime = QTime(0,0);
	QDateTime toDate = QDateTime::currentDateTime();
	QTime toTime = QTime::currentTime();
	QStringList tags;
	QStringList people;
	QStringList location;
	QStringList suit;
	QStringList dnotes;
	QStringList equipment;
	Mode tagsMode = Mode::ALL_OF;
	Mode peopleMode = Mode::ALL_OF;
	Mode locationMode = Mode::ALL_OF;
	Mode dnotesMode = Mode::ALL_OF;
	Mode suitMode = Mode::ALL_OF;
	Mode equipmentMode = Mode::ALL_OF;
	bool logged = true;
	bool planned = true;
};

class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	bool showDive(const struct dive *d) const;
	int divesDisplayed;
	bool lessThan(const QModelIndex &, const QModelIndex &) const override;
public
slots:
	void myInvalidate();
	void clearFilter();
	void startFilterDiveSite(struct dive_site *ds);
	void stopFilterDiveSite();
	void filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles);
	void resetModel(DiveTripModelBase::Layout layout);
	void filterDataChanged(const FilterData &data);
	void divesAdded(struct dive_trip *, bool, const QVector<dive *> &dives);
	void divesDeleted(struct dive_trip *, bool, const QVector<dive *> &dives);

signals:
	void filterFinished();
	void countsChanged();

private:
	MultiFilterSortModel(QObject *parent = 0);
	struct dive_site *curr_dive_site;
	FilterData filterData;
};

#endif
