/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include "../dive.h"

extern struct tank_info tank_info[100];

CylindersModel::CylindersModel(QObject* parent): QAbstractTableModel(parent)
{
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical) {
		return ret;
	}

	if (role == Qt::DisplayRole) {
		switch(section) {
		case TYPE:
			ret = tr("Type");
			break;
		case SIZE:
			ret = tr("Size");
			break;
		case MAXPRESS:
			ret = tr("MaxPress");
			break;
		case START:
			ret = tr("Start");
			break;
		case END:
			ret = tr("End");
			break;
		case O2:
			ret = tr("O2%");
			break;
		case HE:
			ret = tr("He%");
			break;
		}
	}
	return ret;
}

int CylindersModel::columnCount(const QModelIndex& parent) const
{
	return 7;
}

QVariant CylindersModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_CYLINDERS) {
		return ret;
	}

	dive *d = get_dive(selected_dive);
	cylinder_t& cyl = d->cylinder[index.row()];

	if (role == Qt::DisplayRole) {
		switch(index.column()) {
		case TYPE:
			ret = QString(cyl.type.description);
			break;
		case SIZE:
			ret = cyl.type.size.mliter;
			break;
		case MAXPRESS:
			ret = cyl.type.workingpressure.mbar;
			break;
		case START:
			ret = cyl.start.mbar;
			break;
		case END:
			ret = cyl.end.mbar;
			break;
		case O2:
			ret = cyl.gasmix.o2.permille;
			break;
		case HE:
			ret = cyl.gasmix.he.permille;
			break;
		}
	}
	return ret;
}

int CylindersModel::rowCount(const QModelIndex& parent) const
{
	return 	usedRows[currentDive];
}

void CylindersModel::add(cylinder_t* cyl)
{
	if (usedRows[currentDive] >= MAX_CYLINDERS) {
		free(cyl);
	}

	int row = usedRows[currentDive];

	cylinder_t& cylinder = currentDive->cylinder[row];

	cylinder.end.mbar = cyl->end.mbar;
	cylinder.start.mbar = cyl->start.mbar;

	beginInsertRows(QModelIndex(), row, row);
	usedRows[currentDive]++;
	endInsertRows();
}

void CylindersModel::update()
{
	if (usedRows[currentDive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[currentDive]-1);
		endRemoveRows();
	}

	currentDive = get_dive(selected_dive);
	if (usedRows[currentDive] > 0) {
		beginInsertRows(QModelIndex(), 0, usedRows[currentDive]-1);
		endInsertRows();
	}
}

void CylindersModel::clear()
{
	if (usedRows[currentDive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[currentDive]-1);
		usedRows[currentDive] = 0;
		endRemoveRows();
	}
}

void WeightModel::clear()
{
}

int WeightModel::columnCount(const QModelIndex& parent) const
{
	return 0;
}

QVariant WeightModel::data(const QModelIndex& index, int role) const
{
	return QVariant();
}

int WeightModel::rowCount(const QModelIndex& parent) const
{
	return rows;
}

QVariant WeightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical) {
		return ret;
	}

	switch(section) {
	case TYPE:
		ret = tr("Type");
		break;
	case WEIGHT:
		ret = tr("Weight");
		break;
	}
	return ret;
}

void WeightModel::add(weight_t* weight)
{
}

void WeightModel::update()
{
}

void TankInfoModel::add(const QString& description)
{
	// When the user `creates` a new one on the combobox.
	// for now, empty till dirk cleans the GTK code.
}

void TankInfoModel::clear()
{
}

int TankInfoModel::columnCount(const QModelIndex& parent) const
{
	return 3;
}

QVariant TankInfoModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct tank_info *info = &tank_info[index.row()];

	int ml = info->ml;

	int bar = ((info->psi) ? psi_to_bar(info->psi) : info->bar) * 1000 + 0.5;

	if (info->cuft) {
		double airvolume = cuft_to_l(info->cuft) * 1000.0;
		ml = airvolume / bar_to_atm(bar) + 0.5;
	}
	if (role == Qt::DisplayRole) {
		switch(index.column()) {
			case BAR: ret = bar;
				break;
			case ML: ret = ml;
				break;
			case DESCRIPTION:
				ret = QString(info->name);
				break;
		}
	}
	return ret;
}

QVariant TankInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;

	if (orientation != Qt::Horizontal)
		return ret;

	if (role == Qt::DisplayRole) {
		switch(section) {
			case BAR:
				ret = tr("Bar");
				break;
			case ML:
				ret = tr("Ml");
				break;
			case DESCRIPTION:
				ret = tr("Description");
				break;
		}
	}
	return ret;
}

int TankInfoModel::rowCount(const QModelIndex& parent) const
{
	return rows+1;
}

TankInfoModel::TankInfoModel() : QAbstractTableModel(), rows(-1)
{
	struct tank_info *info = tank_info;
	for (info = tank_info ; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void TankInfoModel::update()
{
	if(rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
	}
	struct tank_info *info = tank_info;
	for (info = tank_info ; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}
