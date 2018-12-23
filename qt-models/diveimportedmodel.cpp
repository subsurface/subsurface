#include "diveimportedmodel.h"
#include "core/qthelper.h"

DiveImportedModel::DiveImportedModel(QObject *o) : QAbstractTableModel(o),
	firstIndex(0),
	lastIndex(-1),
	diveTable(nullptr)
{
}

int DiveImportedModel::columnCount(const QModelIndex&) const
{
	return 3;
}

int DiveImportedModel::rowCount(const QModelIndex&) const
{
	return lastIndex - firstIndex + 1;
}

QVariant DiveImportedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Vertical)
		return QVariant();

	// widgets access the model via index.column(), qml via role.
	int column = section;
	if (role == DateTime || role == Duration || role == Depth) {
		column = role - DateTime;
		role = Qt::DisplayRole;
	}

	if (role == Qt::DisplayRole) {
		switch (column) {
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

	struct dive *d = get_dive_from_table(index.row() + firstIndex, diveTable);
	if (!d)
		return QVariant();

	// widgets access the model via index.column(), qml via role.
	int column = index.column();
	if (role >= DateTime) {
		column = role - DateTime;
		role = Qt::DisplayRole;
	}

	if (role == Qt::DisplayRole) {
		switch (column) {
		case 0:
			return QVariant(get_short_dive_date_string(d->when));
		case 1:
			return QVariant(get_dive_duration_string(d->duration.seconds, tr("h"), tr("min")));
		case 2:
			return QVariant(get_depth_string(d->maxdepth.mm, true, false));
		case 3:
			return checkStates[index.row()];
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
	dataChanged(index(clickedIndex.row(), 0), index(clickedIndex.row(), 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectAll()
{
	std::fill(checkStates.begin(), checkStates.end(), true);
	dataChanged(index(0, 0), index(lastIndex - firstIndex, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectRow(int row)
{
	checkStates[row] = !checkStates[row];
	dataChanged(index(row, 0), index(row, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectNone()
{
	std::fill(checkStates.begin(), checkStates.end(), false);
	dataChanged(index(0, 0), index(lastIndex - firstIndex,0 ), QVector<int>() << Qt::CheckStateRole << Selected);
}

Qt::ItemFlags DiveImportedModel::flags(const QModelIndex &index) const
{
	if (index.column() != 0)
		return QAbstractTableModel::flags(index);
	return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

void DiveImportedModel::clearTable()
{
	if (lastIndex < firstIndex) {
		// just to be sure it's the right numbers
		// but don't call RemoveRows or Qt in debug mode with trigger an ASSERT
		lastIndex = -1;
		firstIndex = 0;
		return;
	}
	beginRemoveRows(QModelIndex(), 0, lastIndex - firstIndex);
	lastIndex = -1;
	firstIndex = 0;
	endRemoveRows();
}

void DiveImportedModel::repopulate(dive_table_t *table, trip_table_t *trips)
{
	beginResetModel();

	diveTable = table;
	tripTable = trips;
	firstIndex = 0;
	lastIndex = diveTable->nr - 1;
	checkStates.resize(diveTable->nr);
	std::fill(checkStates.begin(), checkStates.end(), true);

	endResetModel();
}

// Note: this function is only used from mobile - perhaps move it there or unify.
void DiveImportedModel::recordDives()
{
	if (diveTable->nr == 0)
		// nothing to do, just exit
		return;

	// delete non-selected dives
	int total = diveTable->nr;
	int j = 0;
	for (int i = 0; i < total; i++) {
		if (checkStates[i])
			j++;
		else
			delete_dive_from_table(diveTable, j);
	}

	add_imported_dives(diveTable, tripTable, true, true, false);
}

QHash<int, QByteArray> DiveImportedModel::roleNames() const {
	static QHash<int, QByteArray> roles = {
		{ DateTime, "datetime"},
		{ Depth, "depth"},
		{ Duration, "duration"},
		{ Selected, "selected"}
	};
	return roles;
}
