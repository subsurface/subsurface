/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "common.h"
#include "models.h"
#include "../dive.h"

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
			ret = Qtr_("Type");
			break;
		case SIZE:
			ret = Qtr_("Size");
			break;
		case MAXPRESS:
			ret = Qtr_("MaxPress");
			break;
		case START:
			ret = Qtr_("Start");
			break;
		case END:
			ret = Qtr_("End");
			break;
		case O2:
			ret = Qtr_("O2%");
			break;
		case HE:
			ret = Qtr_("He%");
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

	switch(section){
	case TYPE:
		ret = Qtr_("Type");
		break;
	case WEIGHT:
		ret = Qtr_("Weight");
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
