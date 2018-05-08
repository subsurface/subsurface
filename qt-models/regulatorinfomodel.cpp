// SPDX-License-Identifier: GPL-2.0
#include "qt-models/regulatorinfomodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/gettextfromc.h"

RegInfoModel *RegInfoModel::instance()
{
	static RegInfoModel self;
	return &self;
}

bool RegInfoModel::insertRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(row);
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool RegInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	//WARN: check for Qt::EditRole
	Q_UNUSED(role);
	struct reg_info_t *info = &reg_info[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case SERVICE_INTERVAL_TIME:
		info->service_interval_time_months = value.toInt();
		break;
	case SERVICE_INTERVAL_DIVES:
		info->service_interval_number_of_dives = value.toInt();
		break;
	case LAST_SERVICE:
		info->last_service = value.toLongLong();
		break;
	case NEXT_SERVICE:
		// READ ONLY CELL
		break;
	}
	emit dataChanged(index, index);
	return true;
}

void RegInfoModel::clear()
{
}

QVariant RegInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct reg_info_t *info = &reg_info[index.row()];

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case DESCRIPTION:
			ret = info->name;
			break;
		case SERVICE_INTERVAL_TIME:
			ret = info->service_interval_time_months;
			break;
		case SERVICE_INTERVAL_DIVES:
			ret = info->service_interval_number_of_dives;
			break;
		case LAST_SERVICE:
			ret = (qlonglong) info->last_service;
			//if ()
			//ret = QDateTime::fromMSecsSinceEpoch(info->last_service*1000).toString("yyyy-MM-dd");
			break;
		}
		break;
	}
	return ret;
}

int RegInfoModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows + 1;
}

const QString &RegInfoModel::biggerString() const
{
	return biggerEntry;
}

RegInfoModel::RegInfoModel() : rows(-1)
{
	setHeaderDataStrings(QStringList() << tr("Type") << tr("Service interval time")
	<< tr("Dives btw. services") << tr("Last service") << tr("Next service"));
	struct reg_info_t *info = reg_info;
	for (info = reg_info; info->name; info++, rows++) {
		QString regInfoName = gettextFromC::instance()->tr(info->name);
		if (regInfoName.count() > biggerEntry.count())
			biggerEntry = regInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void RegInfoModel::updateInfo()
{
	struct reg_info_t *info = reg_info;
	beginRemoveRows(QModelIndex(), 0, this->rows);
	endRemoveRows();
	rows = -1;
	for (info = reg_info; info->name; info++, rows++) {
		QString regInfoName = gettextFromC::instance()->tr(info->name);
		if (regInfoName.count() > biggerEntry.count())
			biggerEntry = regInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void RegInfoModel::update()
{
	if (rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
		rows = -1;
	}
	
	struct reg_info_t *info = reg_info;
	for (info = reg_info; info->name; info++, rows++)
		;

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
	
}

