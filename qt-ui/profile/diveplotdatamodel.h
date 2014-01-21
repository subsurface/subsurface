#ifndef DIVEPLOTDATAMODEL_H
#define DIVEPLOTDATAMODEL_H

#include <QAbstractTableModel>

struct dive;
struct plot_data;
struct plot_info;

class DivePlotDataModel : public QAbstractTableModel{
Q_OBJECT
public:
	enum {DEPTH, TIME, PRESSURE, TEMPERATURE, USERENTERED, COLOR, CYLINDERINDEX, SENSOR_PRESSURE, INTERPOLATED_PRESSURE, SAC, COLUMNS};
	explicit DivePlotDataModel(QObject* parent = 0);
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	void clear();
	void setDive(struct dive *d, const plot_info& pInfo);
	plot_data* data();
private:
	int sampleCount;
	plot_data *plotData;
};

#endif