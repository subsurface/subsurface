#include "divecomputermodel.h"
#include "dive.h"
#include "divelist.h"

DiveComputerModel::DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
	dcWorkingMap = dcMap;
	numRows = 0;
}

QVariant DiveComputerModel::data(const QModelIndex &index, int role) const
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());

	QVariant ret;
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case ID:
			ret = QString("0x").append(QString::number(node.deviceId, 16));
			break;
		case MODEL:
			ret = node.model;
			break;
		case NICKNAME:
			ret = node.nickName;
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

int DiveComputerModel::rowCount(const QModelIndex &parent) const
{
	return numRows;
}

void DiveComputerModel::update()
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	int count = values.count();

	if (numRows) {
		beginRemoveRows(QModelIndex(), 0, numRows - 1);
		numRows = 0;
		endRemoveRows();
	}

	if (count) {
		beginInsertRows(QModelIndex(), 0, count - 1);
		numRows = count;
		endInsertRows();
	}
}

Qt::ItemFlags DiveComputerModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.column() == NICKNAME)
		flags |= Qt::ItemIsEditable;
	return flags;
}

bool DiveComputerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	// We should test if the role == Qt::EditRole
	Q_UNUSED(role);

	// WARN: This seems wrong - The values don't are ordered - we need a map from the Key to Index, or something.
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());
	dcWorkingMap.remove(node.model, node);
	node.nickName = value.toString();
	dcWorkingMap.insert(node.model, node);
	emit dataChanged(index, index);
	return true;
}

void DiveComputerModel::remove(const QModelIndex &index)
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());
	dcWorkingMap.remove(node.model, node);
	update();
}

void DiveComputerModel::dropWorkingList()
{
	// how do I prevent the memory leak ?
}

void DiveComputerModel::keepWorkingList()
{
	if (dcList.dcMap != dcWorkingMap)
		mark_divelist_changed(true);
	dcList.dcMap = dcWorkingMap;
}
