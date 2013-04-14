/*
 * divetripmodel.h
 *
 * header file for the divetrip model of Subsurface
 *
 */
#ifndef DIVETRIPMODEL_H
#define DIVETRIPMODEL_H

#include <QAbstractItemModel>

/*! A DiveItem for use with a DiveTripModel
 *
 * A simple class which wraps basic stats for a dive (e.g. duration, depth) and
 * tidies up after it's children. This is done manually as we don't inherit from
 * QObject.
 *
*/
class DiveItem
{
public:
	explicit DiveItem(): number(0), dateTime(QString()), duration(0.0), depth(0.0), location(QString()) {parentItem = 0;}
	explicit DiveItem(int num, QString dt, float, float, QString loc, DiveItem *parent = 0);
	~DiveItem() { qDeleteAll(childlist); }

	int diveNumber() const { return number; }
	QString diveDateTime() const { return dateTime; }
	float diveDuration() const { return duration; }
	float diveDepth() const { return depth; }
	QString diveLocation() const { return location; }

	DiveItem *parent() const { return parentItem; }
	DiveItem *childAt(int row) const { return childlist.value(row); }
	int rowOfChild(DiveItem *child) const { return childlist.indexOf(child); }
	int childCount() const { return childlist.count(); }
	bool hasChildren() const { return !childlist.isEmpty(); }
	QList<DiveItem *> children() const { return childlist; }
	void addChild(DiveItem* item) { item->parentItem = this; childlist << item; } /* parent = self */


private:

	int number;
	QString dateTime;
	float duration;
	float depth;
	QString location;

	DiveItem *parentItem;
	QList <DiveItem*> childlist;

};


enum Column {DIVE_NUMBER, DIVE_DATE_TIME, DIVE_DURATION, DIVE_DEPTH, DIVE_LOCATION, COLUMNS};


/*! An AbstractItemModel for recording dive trip information such as a list of dives.
*
*/
class DiveTripModel : public QAbstractItemModel
{
public:

	DiveTripModel(const QString &filename, QObject *parent = 0);

	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	int rowCount(const QModelIndex &parent) const;

	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex index(int row, int column,
	const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &child) const;

	DiveItem *itemForIndex(const QModelIndex &) const;

private:

	DiveItem *rootItem;
	QString filename;

};

#endif // DIVETRIPMODEL_H
