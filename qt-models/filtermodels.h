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
	bool validFilter = false;
	int minVisibility = 0;
	int maxVisibility = 5;
	int minRating = 0;
	int maxRating = 5;
	double minWaterTemp = 0;
	double maxWaterTemp = 100;
	double minAirTemp = 0;
	double maxAirTemp = 100;
	QDateTime fromDate;
	QTime fromTime;
	QDateTime toDate = QDateTime::currentDateTime();
	QTime toTime = QTime::currentTime();
	QStringList tags;
	QStringList people;
	QStringList location;
	QStringList equipment;
	bool logged = true;
	bool planned = true;
	bool invertFilter;
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

signals:
	void filterFinished();

private:
	MultiFilterSortModel(QObject *parent = 0);
	struct dive_site *curr_dive_site;
	FilterData filterData;
};

#endif
