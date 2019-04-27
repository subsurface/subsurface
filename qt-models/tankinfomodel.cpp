// SPDX-License-Identifier: GPL-2.0
#include "qt-models/tankinfomodel.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"

TankInfoModel *TankInfoModel::instance()
{
	static TankInfoModel self;
	return &self;
}

bool TankInfoModel::insertRows(int, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool TankInfoModel::setData(const QModelIndex &index, const QVariant &value, int)
{
	//WARN Seems wrong, we need to check for role == Qt::EditRole

	if (index.row() < 0 || index.row() > MAX_TANK_INFO - 1)
		return false;

	struct tank_info_t *info = &tank_info[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case ML:
		info->ml = value.toInt();
		break;
	case BAR:
		info->bar = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return true;
}

void TankInfoModel::clear()
{
}

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() < 0 || index.row() > MAX_TANK_INFO - 1) {
		return ret;
	}
	if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		struct tank_info_t *info = &tank_info[index.row()];
		int ml = info->ml;
		double bar = (info->psi) ? psi_to_bar(info->psi) : info->bar;

		if (info->cuft && info->psi)
			ml = lrint(cuft_to_l(info->cuft) * 1000 / bar_to_atm(bar));

		switch (index.column()) {
		case BAR:
			ret = bar * 1000;
			break;
		case ML:
			ret = ml;
			break;
		case DESCRIPTION:
			ret = QString(info->name);
			break;
		}
	}
	return ret;
}

int TankInfoModel::rowCount(const QModelIndex&) const
{
	return rows;
}

TankInfoModel::TankInfoModel()
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("ml") << tr("bar"));
	update();
}

void TankInfoModel::update()
{
	beginResetModel();
	rows = 0;
	for (struct tank_info_t *info = tank_info; info->name && info < tank_info + MAX_TANK_INFO; info++, rows++)
		;
	endResetModel();
}
