// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTEREXTRADATAMODEL_H
#define DIVECOMPUTEREXTRADATAMODEL_H

#include "cleanertablemodel.h"

/* extra data model for additional dive computer data */
class ExtraDataModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		KEY,
		VALUE
	};
	explicit ExtraDataModel(QObject *parent = 0);
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;

	void clear();
	void updateDive();

private:
	int rows;
};

#endif
