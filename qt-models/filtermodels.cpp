// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/divetripmodel.h"

#if !defined(SUBSURFACE_MOBILE)
#include "core/divefilter.h"
#endif

#include <QDebug>
#include <algorithm>

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent)
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

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QAbstractItemModel *m = sourceModel();
	QModelIndex index0 = m->index(source_row, 0, source_parent);
	return m->data(index0, DiveTripModelBase::SHOWN_ROLE).value<bool>();
}

void MultiFilterSortModel::myInvalidate()
{
	QAbstractItemModel *m = sourceModel();
	if (!m)
		return;

	{
		// This marker prevents the UI from getting notifications on selection changes.
		// It is active until the end of the scope.
		// This is actually meant for the undo-commands, so that they can do their work
		// without having the UI updated.
		// Here, it is used because invalidating the filter can generate numerous
		// selection changes, which do full ui reloads. Instead, do that all at once
		// as a consequence of the filterFinished signal right after the local scope.
		auto marker = diveListNotifier.enterCommand();

		DiveFilter *filter = DiveFilter::instance();
		for (int i = 0; i < m->rowCount(QModelIndex()); ++i) {
			QModelIndex idx = m->index(i, 0, QModelIndex());

			if (m->data(idx, DiveTripModelBase::IS_TRIP_ROLE).toBool()) {
				// This is a trip -> loop over all dives and see if any is selected

				bool showTrip = false;
				for (int j = 0; j < m->rowCount(idx); ++j) {
					QModelIndex idx2 = m->index(j, 0, idx);
					dive *d = m->data(idx2, DiveTripModelBase::DIVE_ROLE).value<dive *>();
					if (!d) {
						qWarning("MultiFilterSortModel::myInvalidate(): subitem not a dive!?");
						continue;
					}
					bool show = filter->showDive(d);
					if (show)
						showTrip = true;
					m->setData(idx2, show, DiveTripModelBase::SHOWN_ROLE);
				}
				m->setData(idx, showTrip, DiveTripModelBase::SHOWN_ROLE);
			} else {
				dive *d = m->data(idx, DiveTripModelBase::DIVE_ROLE).value<dive *>();
				bool show = (d != NULL) && filter->showDive(d);
				m->setData(idx, show, DiveTripModelBase::SHOWN_ROLE);
			}
		}

		invalidateFilter();

		// Tell the dive trip model to update the displayed-counts
		DiveTripModelBase::instance()->filterFinished();
		countsChanged();
	}

	emit filterFinished();
}

bool MultiFilterSortModel::updateDive(struct dive *d)
{
	bool newStatus = DiveFilter::instance()->showDive(d);
	return filter_dive(d, newStatus);
}

bool MultiFilterSortModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// Hand sorting down to the source model.
	return DiveTripModelBase::instance()->lessThan(i1, i2);
}

void MultiFilterSortModel::countsChanged()
{
	updateWindowTitle();
}
