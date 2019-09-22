#include "diveimportedmodel.h"
#include "core/qthelper.h"
#include "core/divelist.h"

DiveImportedModel::DiveImportedModel(QObject *o) : QAbstractTableModel(o),
	lastIndex(-1),
	diveTable({ 0 }),
	sitesTable({ 0 })
{
	connect(&thread, &QThread::finished, this, &DiveImportedModel::downloadThreadFinished);
}

int DiveImportedModel::columnCount(const QModelIndex&) const
{
	return 3;
}

int DiveImportedModel::rowCount(const QModelIndex&) const
{
	return lastIndex + 1;
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

	if (index.row() > lastIndex)
		return QVariant();

	struct dive *d = get_dive_from_table(index.row(), &diveTable);
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
	dataChanged(index(0, 0), index(lastIndex, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectRow(int row)
{
	checkStates[row] = !checkStates[row];
	dataChanged(index(row, 0), index(row, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectNone()
{
	std::fill(checkStates.begin(), checkStates.end(), false);
	dataChanged(index(0, 0), index(lastIndex, 0 ), QVector<int>() << Qt::CheckStateRole << Selected);
}

Qt::ItemFlags DiveImportedModel::flags(const QModelIndex &index) const
{
	if (index.column() != 0)
		return QAbstractTableModel::flags(index);
	return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

void DiveImportedModel::clearTable()
{
	if (lastIndex < 0) {
		// just to be sure it's the right numbers
		// but don't call RemoveRows or Qt in debug mode with trigger an ASSERT
		lastIndex = -1;
		return;
	}
	beginRemoveRows(QModelIndex(), 0, lastIndex);
	lastIndex = -1;
	endRemoveRows();
}

void DiveImportedModel::downloadThreadFinished()
{
	beginResetModel();

	// Move the table data from thread to model
	move_dive_table(&thread.downloadTable, &diveTable);
	move_dive_site_table(&thread.diveSiteTable, &sitesTable);

	lastIndex = diveTable.nr - 1;
	checkStates.resize(diveTable.nr);
	std::fill(checkStates.begin(), checkStates.end(), true);

	endResetModel();

	emit downloadFinished();
}

void DiveImportedModel::startDownload()
{
	thread.start();
}

std::pair<struct dive_table, struct dive_site_table> DiveImportedModel::consumeTables()
{
	beginResetModel();

	// Move tables to result
	struct dive_table dives = { 0 };
	struct dive_site_table sites = { 0 };
	move_dive_table(&diveTable, &dives);
	move_dive_site_table(&sitesTable, &sites);

	// Reset indexes
	lastIndex = -1;
	checkStates.clear();

	endResetModel();

	return std::make_pair(dives, sites);
}

int DiveImportedModel::numDives() const
{
	return diveTable.nr;
}

// Delete non-selected dives
void DiveImportedModel::deleteDeselected()
{
	int total = diveTable.nr;
	int j = 0;
	for (int i = 0; i < total; i++) {
		if (checkStates[i]) {
			j++;
		} else {
			beginRemoveRows(QModelIndex(), j, j);
			delete_dive_from_table(&diveTable, j);
			endRemoveRows();
		}
	}
	checkStates.resize(diveTable.nr);
	std::fill(checkStates.begin(), checkStates.end(), true);
}

// Note: this function is only used from mobile - perhaps move it there or unify.
void DiveImportedModel::recordDives()
{
	deleteDeselected();

	if (diveTable.nr == 0)
		// nothing to do, just exit
		return;

	// Consume the tables. They will be consumed by add_imported_dives() anyway.
	// But let's do it in a controlled way with a proper model-reset so that the
	// model doesn't become inconsistent.
	std::pair<struct dive_table, struct dive_site_table> tables = consumeTables();

	// TODO: Might want to let the user select IMPORT_ADD_TO_NEW_TRIP
	add_imported_dives(&tables.first, nullptr, &tables.second, IMPORT_PREFER_IMPORTED | IMPORT_IS_DOWNLOADED);
	free(tables.first.dives);
	free(tables.second.dive_sites);
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
