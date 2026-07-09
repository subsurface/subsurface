// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputerextradatamodel.h"
#include "core/divecomputer.h"
#include "core/metrics.h"
#include <libdivecomputer/parser.h>

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
	std::vector<extra_data> new_items;
	if (dc) {
		new_items.reserve(dc->extra_data.size());
		for (const extra_data &data: dc->extra_data) {
			// Filter out fields that are hoisted to first-class members of
			// struct divecomputer and displayed elsewhere in the UI.
			// "Firmware" and "DC Firmware Version" are legacy aliases used by some older Garmin/Uwatec imports.
			if (data.key != STRING_KEY_SERIAL_NUMBER &&
			    data.key != STRING_KEY_FIRMWARE_VERSION &&
			    data.key != "Firmware" &&
			    data.key != "DC Firmware Version" &&
			    data.key != STRING_KEY_TIMEZONE_OFFSET)
				new_items.push_back(data);
		}
	}

	beginResetModel();
	items = std::move(new_items);
	endResetModel();
}
