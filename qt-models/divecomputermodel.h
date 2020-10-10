// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTERMODEL_H
#define DIVECOMPUTERMODEL_H

#include "qt-models/cleanertablemodel.h"
#include "core/device.h"
#include <QSortFilterProxyModel>

class DiveComputerModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum {
		REMOVE,
		MODEL,
		ID,
		NICKNAME
	};
	DiveComputerModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	void keepWorkingList();

public
slots:
	void remove(const QModelIndex &index);

private:
	QVector<device> dcs;
};

class DiveComputerSortedModel : public QSortFilterProxyModel {
public:
	using QSortFilterProxyModel::QSortFilterProxyModel;
private:
	bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const;
};

#endif
