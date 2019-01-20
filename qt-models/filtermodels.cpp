// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "qt-models/models.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/divetripmodel.h"

#if !defined(SUBSURFACE_MOBILE)
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/mainwindow.h"
#endif

#include <QDebug>
#include <algorithm>

namespace {
	// Check if a string-list contains at least one string containing the second argument.
	// Comparison is non case sensitive and removes white space.
	bool listContainsSuperstring(const QStringList &list, const QString &s)
	{
		return std::any_of(list.begin(), list.end(), [&s](const QString &s2)
				   { return s2.trimmed().contains(s.trimmed(), Qt::CaseInsensitive); });
	}

	bool hasTags(const QStringList &tags, const struct dive *d)
	{
		if (tags.isEmpty())
			return true;
		QStringList dive_tags = get_taglist_string(d->tag_list).split(",");

		return std::all_of(tags.begin(), tags.end(), [&dive_tags](const QString &tag)
				   { return listContainsSuperstring(dive_tags, tag); });
	}

	bool hasPersons(const QStringList &people, const struct dive *d)
	{
		if (people.isEmpty())
			return true;
		QStringList dive_people = QString(d->buddy).split(",", QString::SkipEmptyParts)
			+ QString(d->divemaster).split(",", QString::SkipEmptyParts);

		return std::all_of(people.begin(), people.end(), [&dive_people](const QString &person)
				   { return listContainsSuperstring(dive_people, person); });
	}

	bool hasLocations(const QStringList &locations, const struct dive *d)
	{
		if (locations.isEmpty())
			return true;
		QStringList diveLocations;
		if (d->divetrip)
			diveLocations.push_back(QString(d->divetrip->location));

		if (d->dive_site)
			diveLocations.push_back(QString(d->dive_site->name));

		return std::all_of(locations.begin(), locations.end(), [&diveLocations](const QString &location)
				   { return listContainsSuperstring(diveLocations, location); });
	}

	// TODO: Finish this iimplementation.
	bool hasEquipment(const QStringList& equipment, const struct dive *d)
	{
		return true;
	}
}

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent),
	divesDisplayed(0),
	curr_dive_site(NULL)
{
	setFilterKeyColumn(-1); // filter all columns
	setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void MultiFilterSortModel::resetModel(DiveTripModelBase::Layout layout)
{
	DiveTripModelBase::resetModel(layout);
	// DiveTripModelBase::resetModel() generates a new instance.
	// Thus, the source model must be reset.
	setSourceModel(DiveTripModelBase::instance());
}

bool MultiFilterSortModel::showDive(const struct dive *d) const
{
	// If curr_dive_site is set, we are in a special dive-site editing mode.
	if (curr_dive_site)
		return d->dive_site == curr_dive_site;

	if (!filterData.validFilter)
		return true;

	if (d->visibility < filterData.minVisibility || d->visibility > filterData.maxVisibility)
		return false;

	if (d->rating < filterData.minRating || d->rating > filterData.maxRating)
		return false;

	// TODO: get the preferences for the imperial vs metric data.
	// ignore the check if it doesn't makes sense.
	if (d->watertemp.mkelvin &&
	    (d->watertemp.mkelvin < C_to_mkelvin(filterData.minWaterTemp) || d->watertemp.mkelvin > C_to_mkelvin((filterData.maxWaterTemp))))
		return false;

	if (d->airtemp.mkelvin &&
	    (d->airtemp.mkelvin < C_to_mkelvin(filterData.minAirTemp) || d->airtemp.mkelvin > C_to_mkelvin(filterData.maxAirTemp)))
		return false;

	QDateTime t = filterData.fromDate;
	t.setTime(filterData.fromTime);
	if (filterData.fromDate.isValid() && filterData.fromTime.isValid() &&
	    d->when < t.toMSecsSinceEpoch()/1000 + t.offsetFromUtc())
		return false;

	t = filterData.toDate;
	t.setTime(filterData.toTime);
	if (filterData.toDate.isValid() && filterData.toTime.isValid() &&
	    d->when > t.toMSecsSinceEpoch()/1000 + t.offsetFromUtc())
		return false;

	// tags.
	if (!hasTags(filterData.tags, d))
		return false;

	// people
	if (!hasPersons(filterData.people, d))
		return false;

	// Location
	if (!hasLocations(filterData.location, d))
		return false;

	if (!hasEquipment(filterData.equipment, d))
		return false;

	// Planned/Logged
	if (!filterData.logged && !has_planned(d, true))
		return false;
	if (!filterData.planned && !has_planned(d, false))
		return false;

	return true;
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
	struct dive *d = sourceModel()->data(index0, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();

	// For dives, simply check the hidden_by_filter flag
	if (d)
		return !d->hidden_by_filter;

	// Since this is not a dive, it must be a trip
	dive_trip *trip = sourceModel()->data(index0, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();

	if (!trip)
		return false; // Oops. Neither dive nor trip, something is seriously wrong.

	// Show the trip if any dive is visible
	for (int i = 0; i < trip->dives.nr; ++i) {
		if (!trip->dives.dives[i]->hidden_by_filter)
			return true;
	}
	return false;
}

void MultiFilterSortModel::filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles)
{
	// Only redo the filter if a checkbox changed. If the count of an entry changed,
	// we do *not* want to recalculate the filters.
	if (roles.contains(Qt::CheckStateRole))
		myInvalidate();
}

void MultiFilterSortModel::myInvalidate()
{
	int i;
	struct dive *d;

	divesDisplayed = 0;

	// Apply filter for each dive
	for_each_dive (i, d) {
		bool show = showDive(d);
		filter_dive(d, show);
		if (show)
			divesDisplayed++;
	}

	invalidateFilter();

	// Tell the dive trip model to update the displayed-counts
	DiveTripModelBase::instance()->filterFinished();
	emit filterFinished();

#if !defined(SUBSURFACE_MOBILE)
	if (curr_dive_site)
		MainWindow::instance()->diveList->expandAll();
#endif
}

void MultiFilterSortModel::clearFilter()
{
	myInvalidate();
}

void MultiFilterSortModel::startFilterDiveSite(struct dive_site *ds)
{
	curr_dive_site = ds;
	myInvalidate();
}

void MultiFilterSortModel::stopFilterDiveSite()
{
	curr_dive_site = NULL;
	myInvalidate();
}

bool MultiFilterSortModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// Hand sorting down to the source model.
	return DiveTripModelBase::instance()->lessThan(i1, i2);
}

void MultiFilterSortModel::filterDataChanged(const FilterData &data)
{
	filterData = data;
	myInvalidate();
}
