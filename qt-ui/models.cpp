/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include "../dive.h"
#include "../divelist.h"

extern struct tank_info tank_info[100];

CylindersModel::CylindersModel(QObject* parent): QAbstractTableModel(parent)
{
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical) {
		return ret;
	}

	if (role == Qt::DisplayRole) {
		switch(section) {
		case TYPE:
			ret = tr("Type");
			break;
		case SIZE:
			ret = tr("Size");
			break;
		case MAXPRESS:
			ret = tr("MaxPress");
			break;
		case START:
			ret = tr("Start");
			break;
		case END:
			ret = tr("End");
			break;
		case O2:
			ret = tr("O2%");
			break;
		case HE:
			ret = tr("He%");
			break;
		}
	}
	return ret;
}

int CylindersModel::columnCount(const QModelIndex& parent) const
{
	return 7;
}

QVariant CylindersModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_CYLINDERS) {
		return ret;
	}

	dive *d = get_dive(selected_dive);
	cylinder_t& cyl = d->cylinder[index.row()];

	if (role == Qt::DisplayRole) {
		switch(index.column()) {
		case TYPE:
			ret = QString(cyl.type.description);
			break;
		case SIZE:
			ret = cyl.type.size.mliter;
			break;
		case MAXPRESS:
			ret = cyl.type.workingpressure.mbar;
			break;
		case START:
			ret = cyl.start.mbar;
			break;
		case END:
			ret = cyl.end.mbar;
			break;
		case O2:
			ret = cyl.gasmix.o2.permille;
			break;
		case HE:
			ret = cyl.gasmix.he.permille;
			break;
		}
	}
	return ret;
}

int CylindersModel::rowCount(const QModelIndex& parent) const
{
	return 	usedRows[currentDive];
}

void CylindersModel::add(cylinder_t* cyl)
{
	if (usedRows[currentDive] >= MAX_CYLINDERS) {
		free(cyl);
	}

	int row = usedRows[currentDive];

	cylinder_t& cylinder = currentDive->cylinder[row];

	cylinder.end.mbar = cyl->end.mbar;
	cylinder.start.mbar = cyl->start.mbar;

	beginInsertRows(QModelIndex(), row, row);
	usedRows[currentDive]++;
	endInsertRows();
}

void CylindersModel::update()
{
	if (usedRows[currentDive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[currentDive]-1);
		endRemoveRows();
	}

	currentDive = get_dive(selected_dive);
	if (usedRows[currentDive] > 0) {
		beginInsertRows(QModelIndex(), 0, usedRows[currentDive]-1);
		endInsertRows();
	}
}

void CylindersModel::clear()
{
	if (usedRows[currentDive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[currentDive]-1);
		usedRows[currentDive] = 0;
		endRemoveRows();
	}
}

void WeightModel::clear()
{
}

int WeightModel::columnCount(const QModelIndex& parent) const
{
	return 0;
}

QVariant WeightModel::data(const QModelIndex& index, int role) const
{
	return QVariant();
}

int WeightModel::rowCount(const QModelIndex& parent) const
{
	return rows;
}

QVariant WeightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical) {
		return ret;
	}

	switch(section) {
	case TYPE:
		ret = tr("Type");
		break;
	case WEIGHT:
		ret = tr("Weight");
		break;
	}
	return ret;
}

void WeightModel::add(weight_t* weight)
{
}

void WeightModel::update()
{
}

void TankInfoModel::add(const QString& description)
{
	// When the user `creates` a new one on the combobox.
	// for now, empty till dirk cleans the GTK code.
}

void TankInfoModel::clear()
{
}

int TankInfoModel::columnCount(const QModelIndex& parent) const
{
	return 3;
}

QVariant TankInfoModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct tank_info *info = &tank_info[index.row()];

	int ml = info->ml;

	int bar = ((info->psi) ? psi_to_bar(info->psi) : info->bar) * 1000 + 0.5;

	if (info->cuft) {
		double airvolume = cuft_to_l(info->cuft) * 1000.0;
		ml = airvolume / bar_to_atm(bar) + 0.5;
	}
	if (role == Qt::DisplayRole) {
		switch(index.column()) {
			case BAR: ret = bar;
				break;
			case ML: ret = ml;
				break;
			case DESCRIPTION:
				ret = QString(info->name);
				break;
		}
	}
	return ret;
}

QVariant TankInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;

	if (orientation != Qt::Horizontal)
		return ret;

	if (role == Qt::DisplayRole) {
		switch(section) {
			case BAR:
				ret = tr("Bar");
				break;
			case ML:
				ret = tr("Ml");
				break;
			case DESCRIPTION:
				ret = tr("Description");
				break;
		}
	}
	return ret;
}

int TankInfoModel::rowCount(const QModelIndex& parent) const
{
	return rows+1;
}

TankInfoModel::TankInfoModel() : QAbstractTableModel(), rows(-1)
{
	struct tank_info *info = tank_info;
	for (info = tank_info ; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void TankInfoModel::update()
{
	if(rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
	}
	struct tank_info *info = tank_info;
	for (info = tank_info ; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

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
	const QString& diveDateTime() const { return dateTime; }
	float diveDuration() const { return duration; }
	float diveDepth() const { return depth; }
	const QString& diveLocation() const { return location; }
	DiveItem *parent() const { return parentItem; }
	const QList<DiveItem *>& children() const { return childlist; }

	void addChild(DiveItem* item) {
		item->parentItem = this;
		childlist.push_back(item);
	} /* parent = self */

private:
	int number;
	QString dateTime;
	float duration;
	float depth;
	QString location;

	DiveItem *parentItem;
	QList <DiveItem*> childlist;

};

DiveItem::DiveItem(int num, QString dt, float dur, float dep, QString loc, DiveItem *p):
		   number(num), dateTime(dt), duration(dur), depth(dep), location(loc), parentItem(p)
{
	if (parentItem)
		parentItem->addChild(this);
}

DiveTripModel::DiveTripModel(QObject *parent) : QAbstractItemModel(parent)
{
	rootItem = new DiveItem;
	int i;
	struct dive *d;

	for_each_dive(i, d) {
		struct tm tm;
		char *buffer;
		utc_mkdate(d->when, &tm);
		buffer = get_dive_date_string(&tm);
		new DiveItem(d->number,
				buffer,
				d->duration.seconds/60.0,
				d->maxdepth.mm/1000.0 ,
				d->location,
				rootItem);
		free(buffer);
	}
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

	DiveItem *item = static_cast<DiveItem*>(index.internalPointer());

	QVariant retVal;
	if (role == Qt::DisplayRole){
		switch (index.column()) {
			case NR:
				retVal = item->diveNumber();
				break;
			case DATE:
				retVal = item->diveDateTime();
				break;
			case DURATION:
				retVal = item->diveDuration();
				break;
			case DEPTH:
				retVal = item->diveDepth();
				break;
			case LOCATION:
				retVal = item->diveLocation();
				break;
		}
	}
	return retVal;
}


QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation != Qt::Horizontal){
		return ret;
	}

	if (role == Qt::DisplayRole) {
		switch(section){
			case NR:
				ret = tr("#");
				break;
			case DATE:
				ret = tr("Date");
				break;
			case RATING:
				ret = UTF8_BLACKSTAR;
				break;
			case DEPTH:
				ret = tr("ft");
				break;
			case DURATION:
				ret = tr("min");
				break;
			case TEMPERATURE:
				ret = UTF8_DEGREE "F";
				break;
			case TOTALWEIGHT:
				ret = tr("lbs");
				break;
			case SUIT:
				ret = tr("Suit");
				break;
			case CYLINDER:
				ret = tr("Cyl");
				break;
			case NITROX:
				ret = "O" UTF8_SUBSCRIPT_2 "%";
				break;
			case SAC:
				ret = tr("SAC");
				break;
			case OTU:
				ret = tr("OTU");
				break;
			case MAXCNS:
				ret = tr("maxCNS");
				break;
			case LOCATION:
				ret = tr("Location");
				break;
		}
	}
	return ret;
}

int DiveTripModel::rowCount(const QModelIndex &parent) const
{
	/* only allow kids in column 0 */
	if (parent.isValid() && parent.column() > 0){
		return 0;
	}
	DiveItem *item = itemForIndex(parent);
	return item ? item->children().count() : 0;
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
	if (DiveItem *item = parentItem->children().at(row)){
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

	return createIndex(parent->children().indexOf(child), 0, parent);
}


DiveItem* DiveTripModel::itemForIndex(const QModelIndex &index) const
{
	if (index.isValid()) {
		DiveItem *item = static_cast<DiveItem*>(index.internalPointer());
		return item;
	}
	return rootItem;
}
