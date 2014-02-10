#include "diveplotdatamodel.h"
#include "dive.h"
#include "display.h"
#include "profile.h"
#include "graphicsview-common.h"
#include "dive.h"
#include "display.h"
#include "divelist.h"
#include <QDebug>

DivePlotDataModel::DivePlotDataModel(QObject* parent) : QAbstractTableModel(parent) , diveId(0)
{
	memset(&pInfo, 0, sizeof(pInfo));
}

int DivePlotDataModel::columnCount(const QModelIndex& parent) const
{
	return COLUMNS;
}

QVariant DivePlotDataModel::data(const QModelIndex& index, int role) const
{
	if ((!index.isValid())||(index.row() >= pInfo.nr))
		return QVariant();

	plot_data item = pInfo.entry[index.row()];
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

const plot_info& DivePlotDataModel::data() const
{
	return pInfo;
}

int DivePlotDataModel::rowCount(const QModelIndex& parent) const
{
	return pInfo.nr;
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
		pInfo.nr = 0;
		endRemoveRows();
	}
}

void DivePlotDataModel::setDive(dive* d, const plot_info& info)
{
	// We need a way to find out if the dive setted is the same old dive, but pointers change,
	// and there's no UUID, for now, just repopulate everything.
	clear();
	struct divecomputer *dc = NULL;

	if (d)
		dc = select_dc(&d->dc);
	diveId = d->id;
	pInfo = info;
	beginInsertRows(QModelIndex(), 0, pInfo.nr-1);
	endInsertRows();
}

int DivePlotDataModel::id() const
{
	return diveId;
}

#define MAX_PPGAS_FUNC( GAS, GASFUNC ) \
double DivePlotDataModel::GASFUNC() \
{ \
	double ret = -1; \
	for(int i = 0, count = rowCount(); i < count; i++){ \
		if (pInfo.entry[i].GAS > ret) \
			ret = pInfo.entry[i].GAS; \
	} \
	return ret; \
}

MAX_PPGAS_FUNC(phe, pheMax);
MAX_PPGAS_FUNC(pn2, pn2Max);
MAX_PPGAS_FUNC(po2, po2Max);

void DivePlotDataModel::emitDataChanged()
{
	emit dataChanged(QModelIndex(), QModelIndex());
}

void DivePlotDataModel::calculateDecompression()
{
	struct dive *d = getDiveById(id());
	struct divecomputer *dc = select_dc(&d->dc);
	init_decompression(d);
	calculate_deco_information(d, dc, &pInfo, FALSE);
	dataChanged(index(0, CEILING), index(pInfo.nr-1, TISSUE_16));
}
