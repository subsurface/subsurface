// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include "divetripmodel.h"

#include <QSortFilterProxyModel>
#include <memory>

// This proxy model sits on top of either a DiveTripList or DiveTripTree model
// and does filtering and/or sorting.
class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	void sort(int column, Qt::SortOrder order) override;

	void resetModel(DiveTripModelBase::Column row, Qt::SortOrder direction);
	void clear();
signals:
	void selectionChanged(const QVector<QModelIndex> &indexes);
	void currentDiveChanged(QModelIndex index);
private slots:
	void selectionChangedSlot(const QVector<QModelIndex> &indexes);
	void currentDiveChangedSlot(QModelIndex index);
private:
	MultiFilterSortModel(QObject *parent = 0);
	std::unique_ptr<DiveTripModelBase> model;
};

#endif
