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
#include "../qthelper.h"

QFont defaultModelFont();

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
	const QString& biggerString() const;
	void clear();
	void update();
private:
	int rows;
	QString biggerEntry;
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
	const QString& biggerString() const;
	void clear();
	void update();
private:
	int rows;
	QString biggerEntry;
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

struct TreeItem {
	Q_DECLARE_TR_FUNCTIONS (TreeItemDT);
public:
	virtual ~TreeItem();

	virtual QVariant data (int column, int role) const;
	int row() const;
	QList<TreeItem*> children;
	TreeItem *parent;
};

struct TripItem;

class TreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	TreeModel(QObject *parent = 0);
	virtual ~TreeModel();

	virtual   QVariant data(const QModelIndex &index, int role) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ int columnCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ QModelIndex parent(const QModelIndex &child) const;

protected:
	int columns;
	TreeItem *rootItem;
};

class DiveTripModel : public TreeModel {
	Q_OBJECT
public:
	enum Column {NR, DATE, RATING, DEPTH, DURATION, TEMPERATURE, TOTALWEIGHT,
		SUIT, CYLINDER, NITROX, SAC, OTU, MAXCNS, LOCATION, COLUMNS };

	enum ExtraRoles{STAR_ROLE = Qt::UserRole + 1, DIVE_ROLE, SORT_ROLE};
	enum Layout{TREE, LIST, CURRENT};

	Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    DiveTripModel(QObject* parent = 0);
	Layout layout() const;
	void setLayout(Layout layout);

private:
	void setupModelData();
	QMap<dive_trip_t*, TripItem*> trips;
	Layout currentLayout;
};

class DiveComputerModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum {REMOVE, MODEL, ID, NICKNAME, COLUMNS};
	DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent = 0);
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	void update();
	void keepWorkingList();
	void dropWorkingList();

public slots:
	void remove(const QModelIndex& index);
private:
	int numRows;
	QMultiMap<QString, DiveComputerNode> dcWorkingMap;
};

class YearlyStatisticsModel : public TreeModel {
	Q_OBJECT
public:
	enum { 	YEAR,DIVES,TOTAL_TIME,AVERAGE_TIME,SHORTEST_TIME,LONGEST_TIME,AVG_DEPTH,MIN_DEPTH,
		MAX_DEPTH,AVG_SAC,MIN_SAC,MAX_SAC,AVG_TEMP,MIN_TEMP,MAX_TEMP,COLUMNS};

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	YearlyStatisticsModel(QObject* parent = 0);
	void update_yearly_stats();
};
#endif
