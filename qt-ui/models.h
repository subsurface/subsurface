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
#include <QStringListModel>
#include <QSortFilterProxyModel>

#include "metrics.h"

#include "../dive.h"
#include "../divelist.h"
#include "../divecomputer.h"

// Encapsulates Boilerplate.
class CleanerTableModel : public QAbstractTableModel {
	Q_OBJECT
public:
	explicit CleanerTableModel(QObject *parent = 0);
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	void setHeaderDataStrings(const QStringList &headers);

private:
	QStringList headers;
};

/* Encapsulates the tank_info global variable
 * to show on Qt's Model View System.*/
class TankInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	static TankInfoModel *instance();

	enum Column {
		DESCRIPTION,
		ML,
		BAR
	};
	TankInfoModel();

	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	const QString &biggerString() const;
	void clear();
public
slots:
	void update();

private:
	int rows;
	QString biggerEntry;
};

/* Encapsulate ws_info */
class WSInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	static WSInfoModel *instance();

	enum Column {
		DESCRIPTION,
		GR
	};
	WSInfoModel();

	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	const QString &biggerString() const;
	void clear();
	void update();
	void updateInfo();

private:
	int rows;
	QString biggerEntry;
};

/* Retrieve the trash icon pixmap, common to most table models */
const QPixmap &trashIcon();

/* Encapsulation of the Cylinder Model, that presents the
 * Current cylinders that are used on a dive. */
class CylindersModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		REMOVE,
		TYPE,
		SIZE,
		WORKINGPRESS,
		START,
		END,
		O2,
		HE,
		DEPTH,
		USE,
		COLUMNS
	};

	explicit CylindersModel(QObject *parent = 0);
	static CylindersModel *instance();
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex &index) const;
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	void passInData(const QModelIndex &index, const QVariant &value);
	void add();
	void clear();
	void updateDive();
	void copyFromDive(struct dive *d);
	cylinder_t *cylinderAt(const QModelIndex &index);
	bool changed;

public
slots:
	void remove(const QModelIndex &index);

private:
	int rows;
};

/* Encapsulation of the Weight Model, that represents
 * the current weights on a dive. */
class WeightModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		REMOVE,
		TYPE,
		WEIGHT
	};

	explicit WeightModel(QObject *parent = 0);
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;
	/*reimp*/ Qt::ItemFlags flags(const QModelIndex &index) const;
	/*reimp*/ bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	void passInData(const QModelIndex &index, const QVariant &value);
	void add();
	void clear();
	void updateDive();
	weightsystem_t *weightSystemAt(const QModelIndex &index);
	bool changed;

public
slots:
	void remove(const QModelIndex &index);

private:
	int rows;
};

/* extra data model for additional dive computer data */
class ExtraDataModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		KEY,
		VALUE
	};
	explicit ExtraDataModel(QObject *parent = 0);
	/*reimp*/ QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	/*reimp*/ int rowCount(const QModelIndex &parent = QModelIndex()) const;

	void clear();
	void updateDive();

private:
	int rows;
};

/*! An AbstractItemModel for recording dive trip information such as a list of dives.
*
*/

struct TreeItem {
	Q_DECLARE_TR_FUNCTIONS(TreeItemDT);

public:
	virtual ~TreeItem();
	TreeItem();
	virtual QVariant data(int column, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	int row() const;
	QList<TreeItem *> children;
	TreeItem *parent;
};

struct DiveItem : public TreeItem {
	enum Column {
		NR,
		DATE,
		RATING,
		DEPTH,
		DURATION,
		TEMPERATURE,
		TOTALWEIGHT,
		SUIT,
		CYLINDER,
		GAS,
		SAC,
		OTU,
		MAXCNS,
		LOCATION,
		COLUMNS
	};

	virtual QVariant data(int column, int role) const;
	int diveId;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	QString displayDate() const;
	QString displayDuration() const;
	QString displayDepth() const;
	QString displayDepthWithUnit() const;
	QString displayTemperature() const;
	QString displayWeight() const;
	QString displaySac() const;
	int weight() const;
};

struct TripItem;

class TreeModel : public QAbstractItemModel {
	Q_OBJECT
public:
	TreeModel(QObject *parent = 0);
	virtual ~TreeModel();
	virtual QVariant data(const QModelIndex &index, int role) const;
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
	enum Column {
		NR,
		DATE,
		RATING,
		DEPTH,
		DURATION,
		TEMPERATURE,
		TOTALWEIGHT,
		SUIT,
		CYLINDER,
		GAS,
		SAC,
		OTU,
		MAXCNS,
		LOCATION,
		COLUMNS
	};

	enum ExtraRoles {
		STAR_ROLE = Qt::UserRole + 1,
		DIVE_ROLE,
		TRIP_ROLE,
		SORT_ROLE,
		DIVE_IDX
	};
	enum Layout {
		TREE,
		LIST,
		CURRENT
	};

	Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	DiveTripModel(QObject *parent = 0);
	Layout layout() const;
	void setLayout(Layout layout);

private:
	void setupModelData();
	QMap<dive_trip_t *, TripItem *> trips;
	Layout currentLayout;
};

class DiveComputerModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum {
		REMOVE,
		MODEL,
		ID,
		NICKNAME
	};
	DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent = 0);
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	void update();
	void keepWorkingList();
	void dropWorkingList();

public
slots:
	void remove(const QModelIndex &index);

private:
	int numRows;
	QMultiMap<QString, DiveComputerNode> dcWorkingMap;
};

class YearlyStatisticsModel : public TreeModel {
	Q_OBJECT
public:
	enum {
		YEAR,
		DIVES,
		TOTAL_TIME,
		AVERAGE_TIME,
		SHORTEST_TIME,
		LONGEST_TIME,
		AVG_DEPTH,
		MIN_DEPTH,
		MAX_DEPTH,
		AVG_SAC,
		MIN_SAC,
		MAX_SAC,
		AVG_TEMP,
		MIN_TEMP,
		MAX_TEMP,
		COLUMNS
	};

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	YearlyStatisticsModel(QObject *parent = 0);
	void update_yearly_stats();
};

/* TablePrintModel:
 * for now we use a blank table model with row items TablePrintItem.
 * these are pretty much the same as DiveItem, but have color
 * properties, as well. perhaps later one a more unified model has to be
 * considered, but the current TablePrintModel idea has to be extended
 * to support variadic column lists and column list orders that can
 * be controlled by the user.
 */
struct TablePrintItem {
	QString number;
	QString date;
	QString depth;
	QString duration;
	QString divemaster;
	QString buddy;
	QString location;
	unsigned int colorBackground;
};

class TablePrintModel : public QAbstractTableModel {
	Q_OBJECT

private:
	QList<struct TablePrintItem *> list;

public:
	~TablePrintModel();
	TablePrintModel();

	int rows, columns;
	void insertRow(int index = -1);
	void callReset();

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
};

/* ProfilePrintModel:
 * this model is used when printing a data table under a profile. it requires
 * some exact usage of setSpan(..) on the target QTableView widget.
 */
class ProfilePrintModel : public QAbstractTableModel {
	Q_OBJECT

private:
	int diveId;
	double fontSize;

public:
	ProfilePrintModel(QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void setDive(struct dive *divePtr);
	void setFontsize(double size);
};

class GasSelectionModel : public QStringListModel {
	Q_OBJECT
public:
	static GasSelectionModel *instance();
	Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
public
slots:
	void repopulate();
};


class LanguageModel : public QAbstractListModel {
	Q_OBJECT
public:
	static LanguageModel *instance();
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

private:
	LanguageModel(QObject *parent = 0);

	QStringList languages;
};

#endif // MODELS_H
