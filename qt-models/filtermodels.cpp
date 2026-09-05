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
	// Work around QTBUG-141830: same Qt 6.10 re-entrancy bug as in
	// DiveSiteSortedModel. See that constructor for a full explanation.
	// Fixed in Qt 6.10.3 / 6.11.0. Remove once minimum Qt version is 6.10.3+.
	connect(model.get(), &QAbstractItemModel::modelAboutToBeReset, this, [this]() {
		m_savedSortColumn = sortColumn();
		m_savedSortOrder = sortOrder();
		sort(-1);
	});
	connect(model.get(), &QAbstractItemModel::modelReset, this, [this]() {
		if (m_savedSortColumn >= 0)
			sort(m_savedSortColumn, m_savedSortOrder);
	});
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
