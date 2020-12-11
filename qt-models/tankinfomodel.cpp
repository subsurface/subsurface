// SPDX-License-Identifier: GPL-2.0
#include "qt-models/tankinfomodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"

TankInfoModel *TankInfoModel::instance()
{
	static TankInfoModel self;
	return &self;
}

bool TankInfoModel::insertRows(int, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, rowCount(), rowCount() + count - 1);
	for (int i = 0; i < count; ++i)
		add_tank_info_metric(&tank_info_table, "", 0, 0);
	endInsertRows();
	return true;
}

bool TankInfoModel::setData(const QModelIndex &index, const QVariant &value, int)
{
	//WARN Seems wrong, we need to check for role == Qt::EditRole

	if (index.row() < 0 || index.row() >= tank_info_table.nr )
		return false;

	struct tank_info &info = tank_info_table.infos[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		free((void *)info.name);
		info.name = strdup(value.toByteArray().data());
		break;
	case ML:
		info.ml = value.toInt();
		break;
	case BAR:
		info.bar = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return true;
}

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= tank_info_table.nr)
		return QVariant();
	if (role == Qt::FontRole)
		return defaultModelFont();
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const struct tank_info &info = tank_info_table.infos[index.row()];
		int ml = info.ml;
		double bar = (info.psi) ? psi_to_bar(info.psi) : info.bar;

		if (info.cuft && info.psi)
			ml = lrint(cuft_to_l(info.cuft) * 1000 / bar_to_atm(bar));

		switch (index.column()) {
		case BAR:
			return bar * 1000;
		case ML:
			return ml;
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

TankInfoModel::TankInfoModel()
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("ml") << tr("bar"));
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &TankInfoModel::update);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &TankInfoModel::update);
	update();
}

void TankInfoModel::update()
{
	beginResetModel();
	endResetModel();
}
