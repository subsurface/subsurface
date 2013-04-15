/*
 * models.h
 *
 * header file for the equipment models of Subsurface
 *
 */
#ifndef MODELS_H
#define MODELS_H

#include <QAbstractTableModel>
#include "../dive.h"

/* Encapsulates the tank_info global variable
 * to show on Qt`s Model View System.*/
class TankInfoModel : public QAbstractTableModel {
Q_OBJECT
public:
	enum { DESCRIPTION, ML, BAR};
	TankInfoModel();

	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;

	void add(const QString& description);
	void clear();
	void update();
private:
	int rows;
};

class CylindersModel : public QAbstractTableModel {
Q_OBJECT
public:
	enum {TYPE, SIZE, MAXPRESS, START, END, O2, HE};

	explicit CylindersModel(QObject* parent = 0);
	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;

	void add(cylinder_t *cyl);
	void clear();
	void update();
private:
	dive *currentDive;

	/* Since the dive doesn`t stores the number of cylinders that
	 * it has ( max 8 ) and since I don`t want to make a
	 * model-for-each-dive, let`s hack this here instead. */
	QMap<dive*, int> usedRows;
};

class WeightModel : public QAbstractTableModel {
	enum{TYPE, WEIGHT};
	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;

	void add(weight_t *weight);
	void clear();
	void update();
private:
	int rows;
};

#endif
