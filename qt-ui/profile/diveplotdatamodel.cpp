#include "diveplotdatamodel.h"
#include "dive.h"
#include "display.h"
#include "profile.h"
#include "graphicsview-common.h"
#include "divelist.h"
#include <QDebug>

DivePlotDataModel::DivePlotDataModel(QObject *parent) : QAbstractTableModel(parent), diveId(0)
{
	memset(&pInfo, 0, sizeof(pInfo));
}

int DivePlotDataModel::columnCount(const QModelIndex &parent) const
{
	return COLUMNS;
}

QVariant DivePlotDataModel::data(const QModelIndex &index, int role) const
{
	if ((!index.isValid()) || (index.row() >= pInfo.nr))
		return QVariant();

	plot_data item = pInfo.entry[index.row()];
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case DEPTH:
			return item.depth;
		case TIME:
			return item.sec;
		case PRESSURE:
			return item.pressure[0];
		case TEMPERATURE:
			return item.temperature;
		case COLOR:
			return item.velocity;
		case USERENTERED:
			return false;
		case CYLINDERINDEX:
			return item.cylinderindex;
		case SENSOR_PRESSURE:
			return item.pressure[0];
		case INTERPOLATED_PRESSURE:
			return item.pressure[1];
		case CEILING:
			return item.ceiling;
		case SAC:
			return item.sac;
		case PN2:
			return item.pressures.n2;
		case PHE:
			return item.pressures.he;
		case PO2:
			return item.pressures.o2;
		case HEARTBEAT:
			return item.heartbeat;
		case AMBPRESSURE:
			return AMB_PERCENTAGE;
		case GFLINE:
			return item.gfline;
		}
	}

	if (role == Qt::DisplayRole && index.column() >= TISSUE_1 && index.column() <= TISSUE_16) {
		return item.ceilings[index.column() - TISSUE_1];
	}

	if (role == Qt::DisplayRole && index.column() >= PERCENTAGE_1 && index.column() <= PERCENTAGE_16) {
		return item.percentages[index.column() - PERCENTAGE_1];
	}

	if (role == Qt::BackgroundRole) {
		switch (index.column()) {
		case COLOR:
			return getColor((color_indice_t)(VELOCITY_COLORS_START_IDX + item.velocity));
		}
	}
	return QVariant();
}

const plot_info &DivePlotDataModel::data() const
{
	return pInfo;
}

int DivePlotDataModel::rowCount(const QModelIndex &parent) const
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
	case DEPTH:
		return tr("Depth");
	case TIME:
		return tr("Time");
	case PRESSURE:
		return tr("Pressure");
	case TEMPERATURE:
		return tr("Temperature");
	case COLOR:
		return tr("Color");
	case USERENTERED:
		return tr("User entered");
	case CYLINDERINDEX:
		return tr("Cylinder index");
	case SENSOR_PRESSURE:
		return tr("Pressure S");
	case INTERPOLATED_PRESSURE:
		return tr("Pressure I");
	case CEILING:
		return tr("Ceiling");
	case SAC:
		return tr("SAC");
	case PN2:
		return tr("pN₂");
	case PHE:
		return tr("pHe");
	case PO2:
		return tr("pO₂");
	case AMBPRESSURE:
		return tr("Ambient pressure");
	}
	if (role == Qt::DisplayRole && section >= TISSUE_1 && section <= TISSUE_16) {
		return QString("Ceiling: %1").arg(section - TISSUE_1);
	}
	if (role == Qt::DisplayRole && section >= PERCENTAGE_1 && section <= PERCENTAGE_16) {
		return QString("Tissue: %1").arg(section - PERCENTAGE_1);
	}
	return QVariant();
}

void DivePlotDataModel::clear()
{
	if (rowCount() != 0) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		pInfo.nr = 0;
		diveId = -1;
		dcNr = -1;
		endRemoveRows();
	}
}

void DivePlotDataModel::setDive(dive *d, const plot_info &info)
{
	clear();
	Q_ASSERT(d != NULL);
	diveId = d->id;
	dcNr = dc_number;
	pInfo = info;
	beginInsertRows(QModelIndex(), 0, pInfo.nr - 1);
	endInsertRows();
}

unsigned int DivePlotDataModel::dcShown() const
{
	return dcNr;
}

#define MAX_PPGAS_FUNC(GAS, GASFUNC)                                  \
	double DivePlotDataModel::GASFUNC()                           \
	{                                                             \
		double ret = -1;                                      \
		for (int i = 0, count = rowCount(); i < count; i++) { \
			if (pInfo.entry[i].pressures.GAS > ret)                 \
				ret = pInfo.entry[i].pressures.GAS;             \
		}                                                     \
		return ret;                                           \
	}

MAX_PPGAS_FUNC(he, pheMax);
MAX_PPGAS_FUNC(n2, pn2Max);
MAX_PPGAS_FUNC(o2, po2Max);

void DivePlotDataModel::emitDataChanged()
{
	emit dataChanged(QModelIndex(), QModelIndex());
}

void DivePlotDataModel::calculateDecompression()
{
	struct divecomputer *dc = select_dc(&displayed_dive);
	init_decompression(&displayed_dive);
	calculate_deco_information(&displayed_dive, dc, &pInfo, false);
	dataChanged(index(0, CEILING), index(pInfo.nr - 1, TISSUE_16));
}
