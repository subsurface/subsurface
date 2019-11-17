// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/divetripmodel.h"

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	connect(&diveListNotifier, &DiveListNotifier::filterReset, this, &MultiFilterSortModel::invalidateFilter);
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

bool MultiFilterSortModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// Hand sorting down to the source model.
	return DiveTripModelBase::instance()->lessThan(i1, i2);
}
