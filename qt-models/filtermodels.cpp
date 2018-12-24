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
	bool hasTag(const QStringList tags, const struct dive *d)
	{
		if (!tags.isEmpty()) {
			auto dive_tags = get_taglist_string(d->tag_list).split(",");
			bool found_tag = false;
			for (const auto& filter_tag : tags)
				for (const auto& dive_tag : dive_tags)
					if (dive_tag.trimmed().toUpper().contains(filter_tag.trimmed().toUpper()))
						found_tag = true;

			return found_tag;
		}
		return true;
	}

	bool hasPerson(const QStringList people, const struct dive *d)
	{
		if (!people.isEmpty()) {
			QStringList dive_people = QString(d->buddy).split(",", QString::SkipEmptyParts)
				+ QString(d->divemaster).split(",", QString::SkipEmptyParts);

			bool found_person = false;
			for(const auto& filter_person : people)
				for(const auto& dive_person : dive_people)
					if (dive_person.trimmed().toUpper().contains(filter_person.trimmed().toUpper()))
						found_person = true;

			return found_person;
		}
		return true;
	}

	bool hasLocation(const QStringList locations, const struct dive *d)
	{
		if (!locations.isEmpty()) {
			QStringList diveLocations;
			if (d->divetrip)
				diveLocations.push_back(QString(d->divetrip->location));

			if (d->dive_site)
				diveLocations.push_back(QString(d->dive_site->name));

			bool found_location = false;
			for (const auto& filter_location : locations)
				for (const auto& dive_location : diveLocations)
					if (dive_location.trimmed().toUpper().contains(filter_location.trimmed().toUpper()))
						found_location = true;

			return found_location;
		}
		return true;
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
	setSourceModel(DiveTripModel::instance());
}

void MultiFilterSortModel::setLayout(DiveTripModel::Layout layout)
{
	DiveTripModel *tripModel = DiveTripModel::instance();
	tripModel->setLayout(layout);	// Note: setLayout() resets the whole model
}

bool MultiFilterSortModel::showDive(const struct dive *d) const
{
	if (!filterData.validFilter)
		return true;

	if (d->visibility < filterData.minVisibility || d->visibility > filterData.maxVisibility)
		return false;

	if (d->rating < filterData.minRating || d->rating > filterData.maxRating)
		return false;

	// TODO: get the preferences for the imperial vs metric data.
	// ignore the check if it doesn't makes sense.
	if (d->watertemp.mkelvin < C_to_mkelvin(filterData.minWaterTemp) || d->watertemp.mkelvin > C_to_mkelvin((filterData.maxWaterTemp)))
		return false;

	if (d->airtemp.mkelvin < C_to_mkelvin(filterData.minAirTemp) || d->airtemp.mkelvin > C_to_mkelvin(filterData.maxAirTemp))
		return false;

	if (filterData.from.isValid() && d->when < filterData.from.toTime_t())
		return false;

	if (filterData.to.isValid() && d->when > filterData.to.toTime_t())
		return false;

	// tags.
	if (!hasTag(filterData.tags, d))
		return false;

	// people
	if (!hasPerson(filterData.people, d))
		return false;

	// Location
	if (!hasLocation(filterData.location, d))
		return false;

	if (!hasEquipment(filterData.equipment, d))
		return false;

	return true;
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
	struct dive *d = sourceModel()->data(index0, DiveTripModel::DIVE_ROLE).value<struct dive *>();

	// For dives, simply check the hidden_by_filter flag
	if (d)
		return !d->hidden_by_filter;

	// Since this is not a dive, it must be a trip
	dive_trip *trip = sourceModel()->data(index0, DiveTripModel::TRIP_ROLE).value<dive_trip *>();

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
	DiveTripModel::instance()->filterFinished();
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
	return DiveTripModel::instance()->lessThan(i1, i2);
}

void MultiFilterSortModel::filterDataChanged(const FilterData& data)
{
	filterData = data;
	myInvalidate();
}
