// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightsysteminfomodel.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/gettextfromc.h"

WSInfoModel *WSInfoModel::instance()
{
	static WSInfoModel self;
	return &self;
}

bool WSInfoModel::insertRows(int, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool WSInfoModel::setData(const QModelIndex &index, const QVariant &value, int)
{
	//WARN: check for Qt::EditRole
	struct ws_info_t *info = &ws_info[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case GR:
		info->grams = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return true;
}

void WSInfoModel::clear()
{
}

QVariant WSInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct ws_info_t *info = &ws_info[index.row()];

	int gr = info->grams;
	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case GR:
			ret = gr;
			break;
		case DESCRIPTION:
			ret = gettextFromC::instance()->tr(info->name);
			break;
		}
		break;
	}
	return ret;
}

int WSInfoModel::rowCount(const QModelIndex&) const
{
	return rows + 1;
}

const QString &WSInfoModel::biggerString() const
{
	return biggerEntry;
}

WSInfoModel::WSInfoModel() : rows(-1)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("kg"));
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::updateInfo()
{
	struct ws_info_t *info = ws_info;
	beginRemoveRows(QModelIndex(), 0, this->rows);
	endRemoveRows();
	rows = -1;
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::update()
{
	if (rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
		rows = -1;
	}
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++)
		;

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}
