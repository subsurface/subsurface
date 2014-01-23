#include "diveplotdatamodel.h"
#include "dive.h"
#include "display.h"
#include "profile.h"
#include "graphicsview-common.h"
#include "dive.h"
#include "display.h"
#include <QDebug>

DivePlotDataModel::DivePlotDataModel(QObject* parent): QAbstractTableModel(parent), sampleCount(0), plotData(NULL)
{

}

int DivePlotDataModel::columnCount(const QModelIndex& parent) const
{
	return COLUMNS;
}

QVariant DivePlotDataModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	plot_data item = plotData[index.row()];
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case DEPTH:		return item.depth;
			case TIME:		return item.sec;
			case PRESSURE:		return item.pressure[0];
			case TEMPERATURE:	return item.temperature;
			case COLOR:		return item.velocity;
			case USERENTERED:	return false;
			case CYLINDERINDEX: return item.cylinderindex;
			case SENSOR_PRESSURE: return item.pressure[0];
			case INTERPOLATED_PRESSURE: return item.pressure[1];
			case CEILING: return item.ceiling;
			case SAC: return item.sac;
			case PN2: return item.pn2;
			case PHE: return item.phe;
			case PO2: return item.po2;
		}
	}

	if (role == Qt::DisplayRole && index.column() >= TISSUE_1 && index.column() <= TISSUE_16){
		return item.ceilings[ index.column() - TISSUE_1];
	}

	if (role == Qt::BackgroundRole) {
		switch (index.column()) {
			case COLOR:	return getColor((color_indice_t)(VELOCITY_COLORS_START_IDX + item.velocity));
		}
	}
	return QVariant();
}

plot_data* DivePlotDataModel::data()
{
	return plotData;
}

int DivePlotDataModel::rowCount(const QModelIndex& parent) const
{
	return sampleCount;
}

QVariant DivePlotDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	switch (section) {
		case DEPTH:		return tr("Depth");
		case TIME:		return tr("Time");
		case PRESSURE:		return tr("Pressure");
		case TEMPERATURE:	return tr("Temperature");
		case COLOR:		return tr("Color");
		case USERENTERED:	return tr("User Entered");
		case CYLINDERINDEX: return tr("Cylinder Index");
		case SENSOR_PRESSURE: return tr("Pressure  S");
		case INTERPOLATED_PRESSURE: return tr("Pressure I");
		case CEILING: return tr("Ceiling");
		case SAC: return tr("SAC");
		case PN2: return tr("PN2");
		case PHE: return tr("PHE");
		case PO2: return tr("PO2");
	}
	if (role == Qt::DisplayRole && section >= TISSUE_1 && section <= TISSUE_16){
		return QString("Ceiling: %1").arg(section - TISSUE_1);
	}
	return QVariant();
}

void DivePlotDataModel::clear()
{
	if (rowCount() != 0) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		endRemoveRows();
	}
}

void DivePlotDataModel::setDive(dive* d,const plot_info& pInfo)
{
	// We need a way to find out if the dive setted is the same old dive, but pointers change,
	// and there's no UUID, for now, just repopulate everything.
	clear();
	struct divecomputer *dc = NULL;

	if (d)
		dc = select_dc(&d->dc);
	diveId = d->id;
	plotData = pInfo.entry;
	sampleCount = pInfo.nr;
	beginInsertRows(QModelIndex(), 0, sampleCount-1);
	endInsertRows();
}

int DivePlotDataModel::id() const
{
	return diveId;
}
