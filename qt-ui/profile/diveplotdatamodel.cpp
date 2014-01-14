#include "diveplotdatamodel.h"
#include "dive.h"
#include "display.h"
#include "profile.h"
#include "graphicsview-common.h"
#include "dive.h"
#include "display.h"
#include <QDebug>

DivePlotDataModel::DivePlotDataModel(QObject* parent): QAbstractTableModel(parent), plotData(NULL), sampleCount(0)
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
	if (role == Qt::DisplayRole){
		switch(index.column()){
			case DEPTH: return item.depth;
			case TIME: return item.sec;
			case PRESSURE: return item.pressure[0];
			case TEMPERATURE: return item.temperature;
			case COLOR: return item.velocity;
			case USERENTERED: return false;
		}
	}
	if (role == Qt::BackgroundRole){
		switch(index.column()){
			case COLOR: return getColor((color_indice_t)(VELOCITY_COLORS_START_IDX + item.velocity));
		}
	}
	return QVariant();
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

	switch(section){
		case DEPTH: return tr("Depth");
		case TIME: return tr("Time");
		case PRESSURE: return tr("Pressure");
		case TEMPERATURE: return tr("Temperature");
		case COLOR: return tr("Color");
		case USERENTERED: return tr("User Entered");
	}
	return QVariant();
}

void DivePlotDataModel::clear()
{
	if(rowCount() != 0){
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

	/* Create the new plot data */
	if (plotData)
		free((void *)plotData);

	plot_info info = pInfo;
	plotData = populate_plot_entries(d, dc, &info); // Create the plot data.
	analyze_plot_info(&info); // Get the Velocity Color information.

	sampleCount = info.nr;
	beginInsertRows(QModelIndex(), 0, sampleCount-1);
	endInsertRows();
}
