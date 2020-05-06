// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divecomputerextradatamodel.h"
#include "core/dive.h"
#include "core/metrics.h"


ExtraDataModel::ExtraDataModel(QObject *parent) : CleanerTableModel(parent),
	rows(0)
{
	//enum Column {KEY, VALUE};
	setHeaderDataStrings(QStringList() << tr("Key") << tr("Value"));
}

void ExtraDataModel::clear()
{
	beginResetModel();
	rows = 0;
	endResetModel();
}

QVariant ExtraDataModel::data(const QModelIndex &index, int role) const
{
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	int i = -1;
	while (ed && ++i < index.row())
		ed = ed->next;
	if (!ed)
		return QVariant();

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::TextAlignmentRole:
		return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
	case Qt::DisplayRole:
		switch (index.column()) {
		case KEY:
			return ed->key;
		case VALUE:
			return ed->value;
		}
		return QVariant();
	}
	return QVariant();
}

int ExtraDataModel::rowCount(const QModelIndex&) const
{
	return rows;
}

void ExtraDataModel::updateDive()
{
	beginResetModel();
	rows = 0;
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	while (ed) {
		rows++;
		ed = ed->next;
	}
	endResetModel();
}
