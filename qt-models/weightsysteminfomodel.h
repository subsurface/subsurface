// SPDX-License-Identifier: GPL-2.0
#ifndef WEIGHTSYSTEMINFOMODEL_H
#define WEIGHTSYSTEMINFOMODEL_H

#include "cleanertablemodel.h"

/* Encapsulate ws_info */
class WSInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	static WSInfoModel *instance();

	enum Column {
		DESCRIPTION,
		GR
	};
	WSInfoModel();

	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	const QString &biggerString() const;
	void clear();
	void update();
	void updateInfo();

private:
	int rows;
	QString biggerEntry;
};

#endif
