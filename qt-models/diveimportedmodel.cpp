#include "diveimportedmodel.h"
#include "core/helpers.h"

DiveImportedModel::DiveImportedModel(QObject *o) : QAbstractTableModel(o),
	firstIndex(0),
	lastIndex(-1),
	checkStates(0)
{
}

int DiveImportedModel::columnCount(const QModelIndex &model) const
{
	Q_UNUSED(model)
	return 3;
}

int DiveImportedModel::rowCount(const QModelIndex &model) const
{
	Q_UNUSED(model)
	return lastIndex - firstIndex + 1;
}

QVariant DiveImportedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();
	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return QVariant(tr("Date/time"));
		case 1:
			return QVariant(tr("Duration"));
		case 2:
			return QVariant(tr("Depth"));
		}
	}
	return QVariant();
}

QVariant DiveImportedModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (index.row() + firstIndex > lastIndex)
		return QVariant();

	struct dive *d = get_dive_from_table(index.row() + firstIndex, &downloadTable);
	if (!d)
		return QVariant();
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case 0:
			return QVariant(get_short_dive_date_string(d->when));
		case 1:
			return QVariant(get_dive_duration_string(d->duration.seconds, tr("h:"), tr("min")));
		case 2:
			return QVariant(get_depth_string(d->maxdepth.mm, true, false));
		}
	}
	if (role == Qt::CheckStateRole) {
		if (index.column() == 0)
			return checkStates[index.row()] ? Qt::Checked : Qt::Unchecked;
	}
	return QVariant();
}

void DiveImportedModel::changeSelected(QModelIndex clickedIndex)
{
	checkStates[clickedIndex.row()] = !checkStates[clickedIndex.row()];
	dataChanged(index(clickedIndex.row(), 0), index(clickedIndex.row(), 0), QVector<int>() << Qt::CheckStateRole);
}

void DiveImportedModel::selectAll()
{
	memset(checkStates, true, lastIndex - firstIndex + 1);
	dataChanged(index(0, 0), index(lastIndex - firstIndex, 0), QVector<int>() << Qt::CheckStateRole);
}

void DiveImportedModel::selectNone()
{
	memset(checkStates, false, lastIndex - firstIndex + 1);
	dataChanged(index(0, 0), index(lastIndex - firstIndex,0 ), QVector<int>() << Qt::CheckStateRole);
}

Qt::ItemFlags DiveImportedModel::flags(const QModelIndex &index) const
{
	if (index.column() != 0)
		return QAbstractTableModel::flags(index);
	return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

void DiveImportedModel::clearTable()
{
	beginRemoveRows(QModelIndex(), 0, lastIndex - firstIndex);
	lastIndex = -1;
	firstIndex = 0;
	endRemoveRows();
}

void DiveImportedModel::setImportedDivesIndexes(int first, int last)
{
	Q_ASSERT(last >= first);
	if (lastIndex >= firstIndex) {
		beginRemoveRows(QModelIndex(), 0, lastIndex - firstIndex);
		endRemoveRows();
	}
	beginInsertRows(QModelIndex(), 0, last - first);
	lastIndex = last;
	firstIndex = first;
	delete[] checkStates;
	checkStates = new bool[last - first + 1];
	memset(checkStates, true, last - first + 1);
	endInsertRows();
}
