#ifndef DIVEPLOTDATAMODEL_H
#define DIVEPLOTDATAMODEL_H

#include <QAbstractTableModel>

#include "display.h"

struct dive;
struct plot_data;
struct plot_info;

class DivePlotDataModel : public QAbstractTableModel {
	Q_OBJECT
public:
	enum {
		DEPTH,
		TIME,
		PRESSURE,
		TEMPERATURE,
		USERENTERED,
		COLOR,
		CYLINDERINDEX,
		SENSOR_PRESSURE,
		INTERPOLATED_PRESSURE,
		SAC,
		CEILING,
		TISSUE_1,
		TISSUE_2,
		TISSUE_3,
		TISSUE_4,
		TISSUE_5,
		TISSUE_6,
		TISSUE_7,
		TISSUE_8,
		TISSUE_9,
		TISSUE_10,
		TISSUE_11,
		TISSUE_12,
		TISSUE_13,
		TISSUE_14,
		TISSUE_15,
		TISSUE_16,
		PERCENTAGE_1,
		PERCENTAGE_2,
		PERCENTAGE_3,
		PERCENTAGE_4,
		PERCENTAGE_5,
		PERCENTAGE_6,
		PERCENTAGE_7,
		PERCENTAGE_8,
		PERCENTAGE_9,
		PERCENTAGE_10,
		PERCENTAGE_11,
		PERCENTAGE_12,
		PERCENTAGE_13,
		PERCENTAGE_14,
		PERCENTAGE_15,
		PERCENTAGE_16,
		PN2,
		PHE,
		PO2,
		HEARTBEAT,
		AMBPRESSURE,
		GFLINE,
		COLUMNS
	};
	explicit DivePlotDataModel(QObject *parent = 0);
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void clear();
	void setDive(struct dive *d, const plot_info &pInfo);
	const plot_info &data() const;
	unsigned int dcShown() const;
	double pheMax();
	double pn2Max();
	double po2Max();
	void emitDataChanged();
	void calculateDecompression();

private:
	struct plot_info pInfo;
	int diveId;
	unsigned int dcNr;
};

#endif // DIVEPLOTDATAMODEL_H
