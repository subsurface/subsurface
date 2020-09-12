// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightmodel.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/weightsysteminfomodel.h"
#ifndef SUBSURFACE_MOBILE
#include "commands/command.h"
#endif

WeightModel::WeightModel(QObject *parent) : CleanerTableModel(parent),
	d(nullptr),
	tempRow(-1),
	tempWS(empty_weightsystem)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Weight"));
	connect(&diveListNotifier, &DiveListNotifier::weightsystemsReset, this, &WeightModel::weightsystemsReset);
	connect(&diveListNotifier, &DiveListNotifier::weightAdded, this, &WeightModel::weightAdded);
	connect(&diveListNotifier, &DiveListNotifier::weightRemoved, this, &WeightModel::weightRemoved);
	connect(&diveListNotifier, &DiveListNotifier::weightEdited, this, &WeightModel::weightEdited);
}

weightsystem_t WeightModel::weightSystemAt(const QModelIndex &index) const
{
	int row = index.row();
	if (row < 0 || row >= d->weightsystems.nr) {
		qWarning("WeightModel: Accessing invalid weightsystem %d (of %d)", row, d->weightsystems.nr);
		return empty_weightsystem;
	}
	return d->weightsystems.weightsystems[index.row()];
}

void WeightModel::clear()
{
	updateDive(nullptr);
}

QVariant WeightModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= d->weightsystems.nr)
		return QVariant();

	weightsystem_t ws = index.row() == tempRow ? tempWS : weightSystemAt(index);

	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::TextAlignmentRole:
		return Qt::AlignCenter;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			return gettextFromC::tr(ws.description);
		case WEIGHT:
			return get_weight_string(ws.weight, true);
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

// Ownership of passed in weight system will be taken. Caller must not use it any longer.
void WeightModel::setTempWS(int row, weightsystem_t ws)
{
	if (!d || row < 0 || row >= d->weightsystems.nr) // Sanity check: row must exist
		return;

	clearTempWS(); // Shouldn't be necessary, just in case: Reset old temporary row.

	// It is really hard to get the editor-close-hints and setModelData calls under
	// control. Therefore, if the row is set to the already existing entry, don't
	// enter temporary mode.
	const weightsystem_t &oldWS = d->weightsystems.weightsystems[row];
	if (same_string(oldWS.description, ws.description)) {
		free_weightsystem(ws);
	} else {
		tempRow = row;
		tempWS = ws;

		// If the user had already set a weight, don't overwrite that
		if (oldWS.weight.grams && !oldWS.auto_filled)
			tempWS.weight = oldWS.weight;
		else
			tempWS.auto_filled = true;
	}
	dataChanged(index(row, TYPE), index(row, WEIGHT));
}

void WeightModel::clearTempWS()
{
	if (tempRow < 0)
		return;
	int oldRow = tempRow;
	tempRow = -1;
	free_weightsystem(tempWS);
	dataChanged(index(oldRow, TYPE), index(oldRow, WEIGHT));
}

void WeightModel::commitTempWS()
{
#ifndef SUBSURFACE_MOBILE
	if (tempRow < 0 || !d || tempRow > d->weightsystems.nr)
		return;
	// Only submit a command if the type changed
	weightsystem_t ws = d->weightsystems.weightsystems[tempRow];
	if (!same_string(ws.description, tempWS.description) || gettextFromC::tr(ws.description) != QString(tempWS.description)) {
		int count = Command::editWeight(tempRow, tempWS, false);
		emit divesEdited(count);
	}
	tempRow = -1;
#endif
}

bool WeightModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
#ifndef SUBSURFACE_MOBILE
	QString vString = value.toString();
	weightsystem_t ws = weightSystemAt(index);
	switch (index.column()) {
	case WEIGHT:
		ws.weight = string_to_weight(qPrintable(vString));
		ws.auto_filled = false;
		int count = Command::editWeight(index.row(), ws, false);
		emit divesEdited(count);
		return true;
	}
	return false;
#endif
}

Qt::ItemFlags WeightModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int WeightModel::rowCount(const QModelIndex&) const
{
	return d ? d->weightsystems.nr : 0;
}

void WeightModel::updateDive(dive *dIn)
{
	beginResetModel();
	d = dIn;
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

	// The row was already inserted by the undo command. Just inform the model.
	beginInsertRows(QModelIndex(), pos, pos);
	endInsertRows();
}

void WeightModel::weightRemoved(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	// The row was already deleted by the undo command. Just inform the model.
	beginRemoveRows(QModelIndex(), pos, pos);
	endRemoveRows();
}

void WeightModel::weightEdited(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	dataChanged(index(pos, TYPE), index(pos, WEIGHT));
}
