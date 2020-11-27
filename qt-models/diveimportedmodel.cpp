#include "diveimportedmodel.h"
#include "core/qthelper.h"
#include "core/divelist.h"
#include "commands/command.h"

DiveImportedModel::DiveImportedModel(QObject *o) : QAbstractTableModel(o),
	diveTable(empty_dive_table),
	sitesTable(empty_dive_site_table)
{
	connect(&thread, &QThread::finished, this, &DiveImportedModel::downloadThreadFinished);
}

int DiveImportedModel::columnCount(const QModelIndex&) const
{
	return 3;
}

int DiveImportedModel::rowCount(const QModelIndex&) const
{
	return diveTable.nr;
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

	if (index.row() >= diveTable.nr)
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
	dataChanged(index(0, 0), index(diveTable.nr - 1, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectRow(int row)
{
	checkStates[row] = !checkStates[row];
	dataChanged(index(row, 0), index(row, 0), QVector<int>() << Qt::CheckStateRole << Selected);
}

void DiveImportedModel::selectNone()
{
	std::fill(checkStates.begin(), checkStates.end(), false);
	dataChanged(index(0, 0), index(diveTable.nr - 1, 0 ), QVector<int>() << Qt::CheckStateRole << Selected);
}

Qt::ItemFlags DiveImportedModel::flags(const QModelIndex &index) const
{
	if (index.column() != 0)
		return QAbstractTableModel::flags(index);
	return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

void DiveImportedModel::clearTable()
{
	beginResetModel();
	clear_dive_table(&diveTable);
	clear_dive_site_table(&sitesTable);
	endResetModel();
}

void DiveImportedModel::downloadThreadFinished()
{
	beginResetModel();

	// Move the table data from thread to model
	move_dive_table(&thread.downloadTable, &diveTable);
	move_dive_site_table(&thread.diveSiteTable, &sitesTable);
	deviceTable = std::move(thread.deviceTable);

	checkStates.resize(diveTable.nr);
	std::fill(checkStates.begin(), checkStates.end(), true);

	endResetModel();

	emit downloadFinished();
}

void DiveImportedModel::startDownload()
{
	thread.start();
}

void DiveImportedModel::waitForDownload()
{
	thread.wait();
	downloadThreadFinished();
}

std::tuple<struct dive_table, struct dive_site_table, struct device_table> DiveImportedModel::consumeTables()
{
	beginResetModel();

	// Move tables to result
	struct dive_table dives = empty_dive_table;
	struct dive_site_table sites = empty_dive_site_table;
	struct device_table devices;
	move_dive_table(&diveTable, &dives);
	move_dive_site_table(&sitesTable, &sites);
	devices = std::move(deviceTable);

	// Reset indices
	checkStates.clear();

	endResetModel();

	return std::make_tuple(dives, sites, devices);
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
void DiveImportedModel::recordDives(int flags)
{
	// delete non-selected dives
	deleteDeselected();

	// TODO: use structured bindings once we go C++17
	std::tuple<struct dive_table, struct dive_site_table, struct device_table> tables = consumeTables();
	if (std::get<0>(tables).nr > 0) {
		auto data = thread.data();
		Command::importDives(&std::get<0>(tables), nullptr, &std::get<1>(tables),
				     &std::get<2>(tables), nullptr, flags, data->devName());
	} else {
		clear_dive_site_table(&std::get<1>(tables));
	}
	// The dives and dive sites have been consumed, but the arrays of the tables
	// still exist. Free them.
	free(std::get<0>(tables).dives);
	free(std::get<1>(tables).dive_sites);
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
