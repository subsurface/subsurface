// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightmodel.h"
#include "core/subsurface-string.h"
#include <QScopedValueRollback>
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/qthelper.h"
#include "core/string-format.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/weightsysteminfomodel.h"
#ifndef SUBSURFACE_MOBILE
#include "commands/command.h"
#endif

WeightModel::WeightModel(QObject *parent) : CleanerTableModel(parent),
	d(nullptr),
	tempRow(-1)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << "" << tr("Type") << tr("Weight"));
	connect(&diveListNotifier, &DiveListNotifier::weightsystemsReset, this, &WeightModel::weightsystemsReset);
	connect(&diveListNotifier, &DiveListNotifier::weightAdded, this, &WeightModel::weightAdded);
	connect(&diveListNotifier, &DiveListNotifier::weightRemoved, this, &WeightModel::weightRemoved);
	connect(&diveListNotifier, &DiveListNotifier::weightEdited, this, &WeightModel::weightEdited);
}

weightsystem_t WeightModel::weightSystemAt(const QModelIndex &index) const
{
	int row = index.row();
	if (row < 0 || static_cast<size_t>(row) >= d->weightsystems.size()) {
		qWarning("WeightModel: Accessing invalid weightsystem %d (of %d)", row, static_cast<int>(d->weightsystems.size()));
		return weightsystem_t();
	}
	return d->weightsystems[index.row()];
}

void WeightModel::clear()
{
	updateDive(nullptr);
}

QVariant WeightModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || static_cast<size_t>(index.row()) >= d->weightsystems.size())
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
			return gettextFromC::tr(ws.description.c_str());
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
	// Guard against re-entrant calls: emitting dataChanged causes the view to
	// call setEditorData → QComboBox::setCurrentIndex → textHighlighted signal
	// → testActivationString → setModelData → back into setTempWS.  Without
	// this guard the recursion is unbounded and crashes the application.
	// Use QScopedValueRollback so the flag is always cleared when we return,
	// even if an exception or early return occurs inside.
	if (inSetTempWS)
		return;
	QScopedValueRollback<bool> guard(inSetTempWS, true);

	if (!d || row < 0 || static_cast<size_t>(row) >= d->weightsystems.size()) // Sanity check: row must exist
		return;

	clearTempWS(); // Shouldn't be necessary, just in case: Reset old temporary row.

	// It is really hard to get the editor-close-hints and setModelData calls under
	// control. Therefore, if the row is set to the already existing entry, don't
	// enter temporary mode.
	const weightsystem_t &oldWS = d->weightsystems[row];
	if (oldWS.description != ws.description) {
		tempRow = row;
		tempWS = std::move(ws);

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
	tempWS = weightsystem_t();
	dataChanged(index(oldRow, TYPE), index(oldRow, WEIGHT));
}

void WeightModel::commitTempWS()
{
#ifndef SUBSURFACE_MOBILE
	if (tempRow < 0 || !d || static_cast<size_t>(tempRow) >= d->weightsystems.size()) {
		// Row is no longer valid (e.g. dive changed or row removed while an
		// editor was open). Clear the temporary state so the model does not
		// stay stuck reporting tempWS. Don't emit dataChanged for a row that
		// may not exist any longer.
		tempRow = -1;
		tempWS = weightsystem_t();
		return;
	}
	// Only submit a command if the type changed
	weightsystem_t ws = d->weightsystems[tempRow];
	if (ws.description != tempWS.description || gettextFromC::tr(ws.description.c_str()) != QString::fromStdString(tempWS.description)) {
		// Clear tempRow before the command is executed to avoid re-entrant
		// access via signals emitted during command execution.
		int row = tempRow;
		weightsystem_t newWS = std::move(tempWS);
		tempRow = -1;
		int count = Command::editWeight(row, std::move(newWS), false);
		emit divesEdited(count);
	} else {
		tempRow = -1;
	}
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
		int count = Command::editWeight(index.row(), std::move(ws), false);
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
	return d ? static_cast<int>(d->weightsystems.size()) : 0;
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
