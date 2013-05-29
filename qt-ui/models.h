/*
 * models.h
 *
 * header file for the equipment models of Subsurface
 *
 */
#ifndef MODELS_H
#define MODELS_H

#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QStringList>

#include "../dive.h"
#include "../divelist.h"

/* Encapsulates the tank_info global variable
 * to show on Qt's Model View System.*/
class TankInfoModel : public QAbstractTableModel {
Q_OBJECT
public:
	static TankInfoModel* instance();

	enum Column {DESCRIPTION, ML, BAR};
	TankInfoModel();

	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	void clear();
	void update();
private:
	int rows;
};

/* Encapsulate ws_info */
class WSInfoModel : public QAbstractTableModel {
Q_OBJECT
public:
	static WSInfoModel* instance();

	enum Column {DESCRIPTION, GR};
	WSInfoModel();

	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	void clear();
	void update();
private:
	int rows;

};

/* Encapsulation of the Cylinder Model, that presents the
 * Current cylinders that are used on a dive. */
class CylindersModel : public QAbstractTableModel {
Q_OBJECT
public:
	enum Column {REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, COLUMNS};

	explicit CylindersModel(QObject* parent = 0);
	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex& index) const;
	/*reimp*/ bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

	void passInData(const QModelIndex& index, const QVariant& value);
	void add();
	void clear();
	void update();
	void setDive(struct dive *d);
public slots:
	void remove(const QModelIndex& index);

private:
	struct dive *current;
	int rows;
};

/* Encapsulation of the Weight Model, that represents
 * the current weights on a dive. */
class WeightModel : public QAbstractTableModel {
Q_OBJECT
public:
	enum Column {REMOVE, TYPE, WEIGHT, COLUMNS};

	explicit WeightModel(QObject *parent = 0);
	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int columnCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex& parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex& index) const;
	/*reimp*/ bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

	void passInData(const QModelIndex& index, const QVariant& value);
	void add();
	void clear();
	void update();
	void setDive(struct dive *d);
public slots:
	void remove(const QModelIndex& index);

private:
	struct dive *current;
	int rows;
};

/*! An AbstractItemModel for recording dive trip information such as a list of dives.
*
*/

struct TreeItemDT {
	Q_DECLARE_TR_FUNCTIONS (TreeItemDT);
public:
	enum Column {NR, DATE, RATING, DEPTH, DURATION, TEMPERATURE, TOTALWEIGHT,
				SUIT, CYLINDER, NITROX, SAC, OTU, MAXCNS, LOCATION, COLUMNS };

	enum ExtraRoles{STAR_ROLE = Qt::UserRole + 1, DIVE_ROLE};

	virtual ~TreeItemDT();
	int columnCount() const {
		return COLUMNS;
	};

	virtual QVariant data (int column, int role) const;
	int row() const;
	QList<TreeItemDT *> children;
	TreeItemDT *parent;
};

struct TripItem;

class DiveTripModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	enum Layout{TREE, LIST};

	DiveTripModel(QObject *parent = 0);
	~DiveTripModel();

	/*reimp*/ Qt::ItemFlags flags(const QModelIndex &index) const;
	/*reimp*/ QVariant data(const QModelIndex &index, int role) const;
	/*reimp*/ QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ int columnCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ QModelIndex parent(const QModelIndex &child) const;

	Layout layout() const;
	void setLayout(Layout layout);
private:
	void setupModelData();

	TreeItemDT *rootItem;
	QMap<dive_trip_t*, TripItem*> trips;
	Layout currentLayout;
};

#endif
