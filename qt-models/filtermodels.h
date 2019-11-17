// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERMODELS_H
#define FILTERMODELS_H

#include "divetripmodel.h"

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QDateTime>

#include <stdint.h>
#include <vector>

class MultiFilterSortModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	static MultiFilterSortModel *instance();
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	bool updateDive(struct dive *d); // returns true if visibility status changed
	bool lessThan(const QModelIndex &, const QModelIndex &) const override;

	void resetModel(DiveTripModelBase::Layout layout);
	void myInvalidate();

signals:
	void filterFinished();

private:
	MultiFilterSortModel(QObject *parent = 0);
	// Dive site filtering has priority over other filters
	void countsChanged();
};

#endif
