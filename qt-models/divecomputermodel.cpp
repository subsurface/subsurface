// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputermodel.h"
#include "core/dive.h"
#include "core/divelist.h"

DiveComputerModel::DiveComputerModel(QObject *parent) : CleanerTableModel(parent),
	dcs(dcList.dcs)
{
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
}

QVariant DiveComputerModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= dcs.size())
		return QVariant();
	const DiveComputerNode &node = dcs[index.row()];

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case ID:
			return QString("0x").append(QString::number(node.deviceId, 16));
		case MODEL:
			return node.model;
		case NICKNAME:
			return node.nickName;
		}
	}

	if (index.column() == REMOVE) {
		switch (role) {
		case Qt::DecorationRole:
			return trashIcon();
		case Qt::SizeHintRole:
			return trashIcon().size();
		case Qt::ToolTipRole:
			return tr("Clicking here will remove this dive computer.");
		}
	}
	return QVariant();
}

int DiveComputerModel::rowCount(const QModelIndex&) const
{
	return dcs.size();
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
	if (index.row() < 0 || index.row() >= dcs.size())
		return false;

	DiveComputerNode &node = dcs[index.row()];
	node.nickName = value.toString();
	emit dataChanged(index, index);
	return true;
}

void DiveComputerModel::remove(const QModelIndex &index)
{
	if (index.row() < 0 || index.row() >= dcs.size())
		return;
	beginRemoveRows(QModelIndex(), index.row(), index.row());
	dcs.remove(index.row());
	endRemoveRows();
}

void DiveComputerModel::keepWorkingList()
{
	if (dcList.dcs != dcs)
		mark_divelist_changed(true);
	dcList.dcs = dcs;
}
