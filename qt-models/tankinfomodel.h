// SPDX-License-Identifier: GPL-2.0
#ifndef TANKINFOMODEL_H
#define TANKINFOMODEL_H

#include "cleanertablemodel.h"

/* Encapsulates the tank_info global variable
 * to show on Qt's Model View System.*/
class TankInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		DESCRIPTION,
		ML,
		BAR
	};
	TankInfoModel(QObject *parent);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
};

#endif
