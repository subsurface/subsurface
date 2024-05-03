// SPDX-License-Identifier: GPL-2.0
#include "qt-models/tankinfomodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= (int)tank_info_table.size())
		return QVariant();
	if (role == Qt::FontRole)
		return defaultModelFont();
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const struct tank_info &info = tank_info_table[index.row()];
		auto [size, pressure] = extract_tank_info(info);

		switch (index.column()) {
		case BAR:
			return pressure.mbar;
		case ML:
			return size.mliter;
		case DESCRIPTION:
			return QString::fromStdString(info.name);
		}
	}
	return QVariant();
}

int TankInfoModel::rowCount(const QModelIndex&) const
{
	return (int)tank_info_table.size();
}

TankInfoModel::TankInfoModel(QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("ml") << tr("bar"));
}
