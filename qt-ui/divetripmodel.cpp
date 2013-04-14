/*
 * divetripmodel.cpp
 *
 * classes for the dive trip list in Subsurface
 */

#include "common.h"
#include "divetripmodel.h"


DiveItem::DiveItem(int num, QString dt, float dur, float dep, QString loc, DiveItem *p):
		   number(num), dateTime(dt), duration(dur), depth(dep), location(loc), parentItem(p)
{
	if (parentItem)
		parentItem->addChild(this);
}


DiveTripModel::DiveTripModel(const QString &filename, QObject *parent) : QAbstractItemModel(parent), filename(filename)
{
	rootItem = new DiveItem;
}


Qt::ItemFlags DiveTripModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags diveFlags = QAbstractItemModel::flags(index);
	if (index.isValid()) {
		diveFlags |= Qt::ItemIsSelectable|Qt::ItemIsEnabled;
	}
	return diveFlags;
}


QVariant DiveTripModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	DiveItem *item = static_cast<DiveItem*>(index.internalPointer());

	QVariant retVal;
	switch (index.column()) {
	case DIVE_NUMBER:
		retVal = QVariant(item->diveNumber());
		break;
	case DIVE_DATE_TIME:
		retVal = QVariant(item->diveDateTime());
		break;
	case DIVE_DURATION:
		retVal = QVariant(item->diveDuration());
		break;
	case DIVE_DEPTH:
		retVal = QVariant(item->diveDepth());
		break;
	case DIVE_LOCATION:
		retVal = QVariant(item->diveLocation());
		break;
	default:
		return QVariant();
	};
	return retVal;
}


QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section == DIVE_NUMBER) {
			return Qtr_("Dive number");
		} else if (section == DIVE_DATE_TIME) {
			return Qtr_("Date");
		} else if (section == DIVE_DURATION) {
			return Qtr_("Duration");
		} else if (section == DIVE_DEPTH) {
			return Qtr_("Depth");
		} else if (section == DIVE_LOCATION) {
			return Qtr_("Location");
		}
	}
	return QVariant();
}

int DiveTripModel::rowCount(const QModelIndex &parent) const
{
	/* only allow kids in column 0 */
	if (parent.isValid() && parent.column() > 0){
		return 0;
	}
	DiveItem *item = itemForIndex(parent);
	return item ? item->childCount() : 0;
}



int DiveTripModel::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() && parent.column() != 0 ? 0 : COLUMNS;
}


QModelIndex DiveTripModel::index(int row, int column, const QModelIndex &parent) const
{

	if (!rootItem || row < 0 || column < 0 || column >= COLUMNS ||
	    (parent.isValid() && parent.column() != 0))
		return QModelIndex();

	DiveItem *parentItem = itemForIndex(parent);
	Q_ASSERT(parentItem);
	if (DiveItem *item = parentItem->childAt(row)){
		return createIndex(row, column, item);
	}
	return QModelIndex();
}


QModelIndex DiveTripModel::parent(const QModelIndex &childIndex) const
{
	if (!childIndex.isValid())
		return QModelIndex();

	DiveItem *child = static_cast<DiveItem*>(childIndex.internalPointer());
	DiveItem *parent = child->parent();

	if (parent == rootItem)
		return QModelIndex();

	return createIndex(parent->rowOfChild(child), 0, parent);
}


DiveItem* DiveTripModel::itemForIndex(const QModelIndex &index) const
{
	if (index.isValid()) {
		DiveItem *item = static_cast<DiveItem*>(index.internalPointer());
		return item;
	}
	return rootItem;
}
