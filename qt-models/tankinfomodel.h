// SPDX-License-Identifier: GPL-2.0
#ifndef TANKINFOMODEL_H
#define TANKINFOMODEL_H

#include "cleanertablemodel.h"

/* Encapsulates the tank_info global variable
 * to show on Qt's Model View System.*/
class TankInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	static TankInfoModel *instance();

	enum Column {
		DESCRIPTION,
		ML,
		BAR
	};
	TankInfoModel();

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
public
slots:
	void update();
};

#endif
