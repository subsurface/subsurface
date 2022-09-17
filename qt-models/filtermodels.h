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
	bool lessThan(const QModelIndex &, const QModelIndex &) const override;

	void resetModel(DiveTripModelBase::Layout layout);
signals:
	void divesSelected(const QVector<QModelIndex> &indices, QModelIndex currentDive, int currentDC); // currentDC < 0 -> keep DC.
	void tripSelected(QModelIndex trip, QModelIndex currentDive);
private slots:
	void divesSelectedSlot(const QVector<QModelIndex> &indices, QModelIndex currentDive, int currentDC);
	void tripSelectedSlot(QModelIndex trip, QModelIndex currentDive);
private:
	MultiFilterSortModel(QObject *parent = 0);
	std::unique_ptr<DiveTripModelBase> model;
};

#endif
