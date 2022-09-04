// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/divetripmodel.h"
#include "qt-models/divelocationmodel.h"

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent)
{
	resetModel(DiveTripModelBase::TREE);
}

void MultiFilterSortModel::resetModel(DiveTripModelBase::Layout layout)
{
	if (layout == DiveTripModelBase::TREE)
		model.reset(new DiveTripModelTree);
	else
		model.reset(new DiveTripModelList);

	setSourceModel(model.get());
	connect(model.get(), &DiveTripModelBase::divesSelected, this, &MultiFilterSortModel::divesSelectedSlot);
	connect(model.get(), &DiveTripModelBase::tripSelected, this, &MultiFilterSortModel::tripSelectedSlot);
	model->initSelection();
	LocationInformationModel::instance()->update();
}

// Translate selection into local indices and re-emit signal
void MultiFilterSortModel::divesSelectedSlot(const QVector<QModelIndex> &indices, QModelIndex currentDive, int currentDC)
{
	QVector<QModelIndex> indicesLocal;
	indicesLocal.reserve(indices.size());
	for (const QModelIndex &index: indices) {
		QModelIndex local = mapFromSource(index);
		if (local.isValid())
			indicesLocal.push_back(local);
	}

	emit divesSelected(indicesLocal, mapFromSource(currentDive), currentDC);
}

// Translate selection into local indices and re-emit signal
void MultiFilterSortModel::tripSelectedSlot(QModelIndex trip, QModelIndex currentDive)
{
	QModelIndex local = mapFromSource(trip);
	if (!local.isValid())
		return;

	emit tripSelected(local, mapFromSource(currentDive));
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	return true;
}

bool MultiFilterSortModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// Hand sorting down to the source model.
	return model->lessThan(i1, i2);
}
