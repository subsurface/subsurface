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
	Mode locationMode = Mode::ANY_OF;
	Mode dnotesMode = Mode::ALL_OF;
	Mode suitMode = Mode::ANY_OF;
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
	bool updateDive(struct dive *d); // returns true if visibility status changed
	int divesDisplayed;
	bool lessThan(const QModelIndex &, const QModelIndex &) const override;
	bool diveSiteMode() const; // returns true if we're filtering on dive site
	const QVector<dive_site *> &filteredDiveSites() const;
public
slots:
	void myInvalidate();
	void clearFilter();
	void startFilterDiveSites(QVector<dive_site *> ds);
	void setFilterDiveSite(QVector<dive_site *> ds);
	void stopFilterDiveSites();
	void filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles);
	void resetModel(DiveTripModelBase::Layout layout);
	void filterDataChanged(const FilterData &data);
	void divesAdded(struct dive_trip *, bool, const QVector<dive *> &dives);
	void divesDeleted(struct dive_trip *, bool, const QVector<dive *> &dives);

signals:
	void filterFinished();

private:
	MultiFilterSortModel(QObject *parent = 0);
	// Dive site filtering has priority over other filters
	QVector<dive_site *> dive_sites;
	void countsChanged();
	FilterData filterData;

	// We use ref-counting for the dive site mode. The reason is that when switching
	// between two tabs that both need dive site mode, the following course of
	// events may happen:
	//	1) The new tab appears -> enter dive site mode.
	//	2) The old tab gets its hide() signal -> exit dive site mode.
	// The filter is now not in dive site mode, even if it should
	int diveSiteRefCount;
};

#endif
