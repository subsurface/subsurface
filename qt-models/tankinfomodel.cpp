// SPDX-License-Identifier: GPL-2.0
#include "qt-models/tankinfomodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= tank_info_table.nr)
		return QVariant();
	if (role == Qt::FontRole)
		return defaultModelFont();
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const struct tank_info &info = tank_info_table.infos[index.row()];
		volume_t size = {0};
		pressure_t pressure = {0};
		extract_tank_info(&info, &size, &pressure);

		switch (index.column()) {
		case BAR:
			return pressure.mbar;
		case ML:
			return size.mliter;
		case DESCRIPTION:
			return info.name;
		}
	}
	return QVariant();
}

int TankInfoModel::rowCount(const QModelIndex&) const
{
	return tank_info_table.nr;
}

TankInfoModel::TankInfoModel(QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("ml") << tr("bar"));
}
