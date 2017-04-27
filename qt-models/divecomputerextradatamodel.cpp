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
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

QVariant ExtraDataModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	int i = -1;
	while (ed && ++i < index.row())
		ed = ed->next;
	if (!ed)
		return ret;

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = int(Qt::AlignLeft | Qt::AlignVCenter);
		break;
	case Qt::DisplayRole:
		switch (index.column()) {
		case KEY:
			ret = QString(ed->key);
			break;
		case VALUE:
			ret = QString(ed->value);
			break;
		}
		break;
	}
	return ret;
}

int ExtraDataModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows;
}

void ExtraDataModel::updateDive()
{
	clear();
	rows = 0;
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	while (ed) {
		rows++;
		ed = ed->next;
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}
