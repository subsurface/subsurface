// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightsysteminfomodel.h"
#include "core/equipment.h"
#include "core/metrics.h"
#include "core/gettextfromc.h"

QVariant WSInfoModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= static_cast<int>(ws_info_table.size()))
		return QVariant();
	const ws_info &info = ws_info_table[index.row()];

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case GR:
			return info.weight.grams;
		case DESCRIPTION:
			// TODO: don't translate user supplied names
			return gettextFromC::tr(info.name.c_str());
		}
		break;
	}
	return QVariant();
}

int WSInfoModel::rowCount(const QModelIndex&) const
{
	return static_cast<int>(ws_info_table.size());
}

WSInfoModel::WSInfoModel(QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("kg"));
}
