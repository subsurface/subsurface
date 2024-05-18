// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputerextradatamodel.h"
#include "core/divecomputer.h"
#include "core/metrics.h"

ExtraDataModel::ExtraDataModel(QObject *parent) : CleanerTableModel(parent)
{
	//enum Column {KEY, VALUE};
	setHeaderDataStrings(QStringList() << tr("Key") << tr("Value"));
}

void ExtraDataModel::clear()
{
	beginResetModel();
	items.clear();
	endResetModel();
}

QVariant ExtraDataModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() > (int)items.size())
		return QVariant();
	const extra_data &item = items[index.row()];

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::TextAlignmentRole:
		return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::DisplayRole:
		switch (index.column()) {
		case KEY:
			return QString::fromStdString(item.key);
		case VALUE:
			return QString::fromStdString(item.value);
		}
		return QVariant();
	}
	return QVariant();
}

int ExtraDataModel::rowCount(const QModelIndex&) const
{
	return (int)items.size();
}

void ExtraDataModel::updateDiveComputer(const struct divecomputer *dc)
{
	beginResetModel();
	if (dc)
		items = dc->extra_data;
	else
		items.clear();
	endResetModel();
}
