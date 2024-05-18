// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTEREXTRADATAMODEL_H
#define DIVECOMPUTEREXTRADATAMODEL_H

#include "cleanertablemodel.h"
#include "core/extradata.h"

struct divecomputer;

/* extra data model for additional dive computer data */
class ExtraDataModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		KEY,
		VALUE
	};
	explicit ExtraDataModel(QObject *parent = 0);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	void clear();
	void updateDiveComputer(const struct divecomputer *dc);

private:
	std::vector<extra_data> items;
};

#endif
