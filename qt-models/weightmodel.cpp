// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightmodel.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/helpers.h"
#include "qt-models/weigthsysteminfomodel.h"

WeightModel::WeightModel(QObject *parent) : CleanerTableModel(parent),
	changed(false),
	rows(0)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Weight"));
}

weightsystem_t *WeightModel::weightSystemAt(const QModelIndex &index)
{
	return &displayed_dive.weightsystem[index.row()];
}

void WeightModel::remove(const QModelIndex &index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_weightsystem(&displayed_dive, index.row());
	changed = true;
	endRemoveRows();
}

void WeightModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

QVariant WeightModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_WEIGHTSYSTEMS)
		return ret;

	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			ret = gettextFromC::instance()->tr(ws->description);
			break;
		case WEIGHT:
			ret = get_weight_string(ws->weight, true);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			ret = trashIcon().size();
		break;
	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this weight system.");
		break;
	}
	return ret;
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void WeightModel::passInData(const QModelIndex &index, const QVariant &value)
{
	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];
	if (index.column() == WEIGHT) {
		if (ws->weight.grams != value.toInt()) {
			ws->weight.grams = value.toInt();
			dataChanged(index, index);
		}
	}
}


bool WeightModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString = value.toString();
	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];
	switch (index.column()) {
	case TYPE:
		if (!value.isNull()) {
			//TODO: C-function weigth_system_set_description ?
			if (!ws->description || gettextFromC::instance()->tr(ws->description) != vString) {
				// loop over translations to see if one matches
				int i = -1;
				while (ws_info[++i].name) {
					if (gettextFromC::instance()->tr(ws_info[i].name) == vString) {
						ws->description = copy_string(ws_info[i].name);
						break;
					}
				}
				if (ws_info[i].name == NULL) // didn't find a match
					ws->description = strdup(vString.toUtf8().constData());
				changed = true;
			}
		}
		break;
	case WEIGHT:
		if (CHANGED()) {
			ws->weight = string_to_weight(vString.toUtf8().data());
			// now update the ws_info
			changed = true;
			WSInfoModel *wsim = WSInfoModel::instance();
			QModelIndexList matches = wsim->match(wsim->index(0, 0), Qt::DisplayRole, gettextFromC::instance()->tr(ws->description));
			if (!matches.isEmpty())
				wsim->setData(wsim->index(matches.first().row(), WSInfoModel::GR), ws->weight.grams);
		}
		break;
	}
	dataChanged(index, index);
	return true;
}

Qt::ItemFlags WeightModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int WeightModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows;
}

void WeightModel::add()
{
	if (rows >= MAX_WEIGHTSYSTEMS)
		return;

	int row = rows;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
	endInsertRows();
}

void WeightModel::updateDive()
{
	clear();
	rows = 0;
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		if (!weightsystem_none(&displayed_dive.weightsystem[i])) {
			rows = i + 1;
		}
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}
