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

	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	const QString &biggerString() const;
	void clear();
public
slots:
	void update();

private:
	int rows;
	QString biggerEntry;
};

#endif
