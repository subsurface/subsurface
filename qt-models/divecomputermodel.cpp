// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputermodel.h"
#include "commands/command.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/subsurface-qt/divelistnotifier.h"

DiveComputerModel::DiveComputerModel(QObject *parent) : CleanerTableModel(parent)
{
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &DiveComputerModel::update);
	connect(&diveListNotifier, &DiveListNotifier::deviceAdded, this, &DiveComputerModel::deviceAdded);
	connect(&diveListNotifier, &DiveListNotifier::deviceDeleted, this, &DiveComputerModel::deviceDeleted);
	connect(&diveListNotifier, &DiveListNotifier::deviceEdited, this, &DiveComputerModel::deviceEdited);
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
	update();
}

void DiveComputerModel::update()
{
	beginResetModel();
	endResetModel();
}

QVariant DiveComputerModel::data(const QModelIndex &index, int role) const
{
	const device *dev = get_device(&device_table, index.row());
	if (dev == nullptr)
		return QVariant();
	const device &node = *dev;

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case ID:
			return QString("0x").append(QString::number(node.deviceId, 16));
		case MODEL:
			return QString::fromStdString(node.model);
		case NICKNAME:
			return QString::fromStdString(node.nickName);
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
	return (int)device_table.devices.size();
}

void DiveComputerModel::deviceAdded(int idx)
{
	beginInsertRows(QModelIndex(), idx, idx);
	endInsertRows();
}

void DiveComputerModel::deviceDeleted(int idx)
{
	beginRemoveRows(QModelIndex(), idx, idx);
	endRemoveRows();
}

void DiveComputerModel::deviceEdited(int idx)
{
	dataChanged(index(idx, REMOVE), index(idx, NICKNAME));
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
	Command::editDeviceNickname(index.row(), value.toString());
	return true;
}

// Convenience function to access alternative columns
static QString getData(const QModelIndex &idx, int col)
{
	const QAbstractItemModel *model = idx.model();
	QModelIndex idx2 = model->index(idx.row(), col, idx.parent());
	return model->data(idx2).toString();
}

// Helper function: sort data pointed to by the given indexes.
// For equal data, sort by two alternative rows.
// All sorting is by case-insensitive string comparison.
static bool sortHelper(const QModelIndex &i1, const QModelIndex &i2, int altRow1, int altRow2)
{
	if(int cmp = i1.data().toString().compare(i2.data().toString()))
		return cmp < 0;
	if(int cmp = getData(i1, altRow1).compare(getData(i2, altRow1)))
		return cmp < 0;
	return getData(i1, altRow2).compare(getData(i2, altRow2)) < 0;
}

bool DiveComputerSortedModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// We assume that i1.column() == i2.column()
	switch (i1.column()) {
		case DiveComputerModel::ID:
			return sortHelper(i1, i2, DiveComputerModel::MODEL, DiveComputerModel::NICKNAME);
		case DiveComputerModel::MODEL:
		default:
			return sortHelper(i1, i2, DiveComputerModel::ID, DiveComputerModel::NICKNAME);
		case DiveComputerModel::NICKNAME:
			return sortHelper(i1, i2, DiveComputerModel::MODEL, DiveComputerModel::ID);
	}
}

void DiveComputerSortedModel::remove(const QModelIndex &index)
{
	int row = mapToSource(index).row();
	if (row < 0 || row >= (int)device_table.devices.size())
		return;
	Command::removeDevice(row);
}
