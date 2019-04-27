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

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
	void clear();
	void update();

private:
	int rows;
	QString biggerEntry;
};

#endif
