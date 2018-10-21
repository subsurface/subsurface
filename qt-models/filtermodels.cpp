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

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent),
	divesDisplayed(0),
	curr_dive_site(NULL),
	model(DiveTripModel::instance())
{
	setFilterKeyColumn(-1); // filter all columns
	setFilterCaseSensitivity(Qt::CaseInsensitive);
	setSourceModel(model);
}

void MultiFilterSortModel::setLayout(DiveTripModel::Layout layout)
{
	DiveTripModel *tripModel = DiveTripModel::instance();
	tripModel->setLayout(layout);	// Note: setLayout() resets the whole model
}

bool MultiFilterSortModel::showDive(const struct dive *d) const
{
	if (curr_dive_site) {
		dive_site *ds = d->dive_site;
		if (!ds)
			return false;
		return ds == curr_dive_site || same_string(ds->name, curr_dive_site->name);
	}

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
	return model->lessThan(i1, i2);
}

void MultiFilterSortModel::filterDataChanged(const FilterData& data)
{
	filterData = data;
	myInvalidate();
}
