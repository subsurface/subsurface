// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputermodel.h"
#include "core/dive.h"
#include "core/divelist.h"

DiveComputerModel::DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
	dcWorkingMap = dcMap;
}

// The backing store of this model is a QMultiMap. But access is done via a sequential
// index. This function turns an index into a map iterator. Not pretty, but still better
// than converting into a list on every access.
template <typename T>
auto getNthElement(T &map, int n) -> decltype(map.begin())
{
	if (n <= 0)
		return map.begin();
	if (n >= map.size())
		return map.end();
	return std::next(map.begin(), n);
}

QVariant DiveComputerModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	auto it = getNthElement(dcWorkingMap, index.row());
	if (it == dcWorkingMap.end())
		return ret;

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case ID:
			ret = QString("0x").append(QString::number(it->deviceId, 16));
			break;
		case MODEL:
			ret = it->model;
			break;
		case NICKNAME:
			ret = it->nickName;
			break;
		}
	}

	if (index.column() == REMOVE) {
		switch (role) {
		case Qt::DecorationRole:
			ret = trashIcon();
			break;
		case Qt::SizeHintRole:
			ret = trashIcon().size();
			break;
		case Qt::ToolTipRole:
			ret = tr("Clicking here will remove this dive computer.");
			break;
		}
	}
	return ret;
}

int DiveComputerModel::rowCount(const QModelIndex&) const
{
	return dcWorkingMap.size();
}

Qt::ItemFlags DiveComputerModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.column() == NICKNAME)
		flags |= Qt::ItemIsEditable;
	return flags;
}

bool DiveComputerModel::setData(const QModelIndex &index, const QVariant &value, int)
{
	// We should test if the role == Qt::EditRole
	auto it = getNthElement(dcWorkingMap, index.row());
	if (it == dcWorkingMap.end())
		return false;
	it->nickName = value.toString();
	emit dataChanged(index, index);
	return true;
}

void DiveComputerModel::remove(const QModelIndex &index)
{
	auto it = getNthElement(dcWorkingMap, index.row());
	if (it == dcWorkingMap.end())
		return;
	beginRemoveRows(QModelIndex(), index.row(), index.row());
	dcWorkingMap.erase(it);
	endRemoveRows();
}

void DiveComputerModel::keepWorkingList()
{
	if (dcList.dcMap != dcWorkingMap)
		mark_divelist_changed(true);
	dcList.dcMap = dcWorkingMap;
}
