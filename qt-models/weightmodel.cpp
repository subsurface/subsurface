// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightmodel.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/weightsysteminfomodel.h"

WeightModel::WeightModel(QObject *parent) : CleanerTableModel(parent),
	changed(false),
	d(nullptr),
	rows(0)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Weight"));
	connect(&diveListNotifier, &DiveListNotifier::weightsystemsReset, this, &WeightModel::weightsystemsReset);
	connect(&diveListNotifier, &DiveListNotifier::weightAdded, this, &WeightModel::weightAdded);
	connect(&diveListNotifier, &DiveListNotifier::weightRemoved, this, &WeightModel::weightRemoved);
}

weightsystem_t *WeightModel::weightSystemAt(const QModelIndex &index)
{
	int row = index.row();
	if (row < 0 || row >= d->weightsystems.nr) {
		qWarning("WeightModel: Accessing invalid weightsystem %d (of %d)", row, d->weightsystems.nr);
		return nullptr;
	}
	return &d->weightsystems.weightsystems[index.row()];
}

void WeightModel::remove(QModelIndex index)
{
	if (index.column() != REMOVE || !d)
		return;
	beginRemoveRows(QModelIndex(), index.row(), index.row());
	rows--;
	remove_weightsystem(d, index.row());
	changed = true;
	endRemoveRows();
}

void WeightModel::clear()
{
	updateDive(nullptr);
}

QVariant WeightModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= d->weightsystems.nr)
		return QVariant();

	const weightsystem_t *ws = &d->weightsystems.weightsystems[index.row()];

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::TextAlignmentRole:
		return Qt::AlignCenter;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			return gettextFromC::tr(ws->description);
		case WEIGHT:
			return get_weight_string(ws->weight, true);
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			return trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			return trashIcon().size();
		break;
	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			return tr("Clicking here will remove this weight system.");
		break;
	}
	return QVariant();
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void WeightModel::passInData(const QModelIndex &index, const QVariant &value)
{
	weightsystem_t *ws = &d->weightsystems.weightsystems[index.row()];
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
	weightsystem_t *ws = &d->weightsystems.weightsystems[index.row()];
	switch (index.column()) {
	case TYPE:
		if (!value.isNull()) {
			//TODO: C-function weight_system_set_description ?
			if (!ws->description || gettextFromC::tr(ws->description) != vString) {
				// loop over translations to see if one matches
				int i = -1;
				while (i < MAX_WS_INFO && ws_info[++i].name) {
					if (gettextFromC::tr(ws_info[i].name) == vString) {
						ws->description = copy_string(ws_info[i].name);
						break;
					}
				}
				if (i == MAX_WS_INFO || ws_info[i].name == NULL) // didn't find a match
					ws->description = copy_qstring(vString);
				changed = true;
			}
		}
		break;
	case WEIGHT:
		if (CHANGED()) {
			ws->weight = string_to_weight(qPrintable(vString));
			// now update the ws_info
			changed = true;
			WSInfoModel *wsim = WSInfoModel::instance();
			QModelIndexList matches = wsim->match(wsim->index(0, 0), Qt::DisplayRole, gettextFromC::tr(ws->description));
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

int WeightModel::rowCount(const QModelIndex&) const
{
	return rows;
}

void WeightModel::updateDive(dive *dIn)
{
	beginResetModel();
	d = dIn;
	rows = d ? d->weightsystems.nr : 0;
	endResetModel();
}

void WeightModel::weightsystemsReset(const QVector<dive *> &dives)
{
	// This model only concerns the currently displayed dive. If this is not among the
	// dives that had their weight reset, exit.
	if (!d || std::find(dives.begin(), dives.end(), d) == dives.end())
		return;

	// And update the model..
	updateDive(d);
}

void WeightModel::weightAdded(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	// The last row was already inserted by the undo command. Just inform the model.
	beginInsertRows(QModelIndex(), pos, pos);
	rows++;
	endInsertRows();
}

void WeightModel::weightRemoved(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	// The row was already deleted by the undo command. Just inform the model.
	beginRemoveRows(QModelIndex(), pos, pos);
	rows--;
	endRemoveRows();
}
