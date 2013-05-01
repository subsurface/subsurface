/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include <QtDebug>

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
	cylinder_t& cyl = current_dive->cylinder[index.row()];

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
	return 	usedRows[current_dive];
}

void CylindersModel::add(cylinder_t* cyl)
{
	if (usedRows[current_dive] >= MAX_CYLINDERS) {
		free(cyl);
	}

	int row = usedRows[current_dive];

	cylinder_t& cylinder = current_dive->cylinder[row];

	cylinder.end.mbar = cyl->end.mbar;
	cylinder.start.mbar = cyl->start.mbar;

	beginInsertRows(QModelIndex(), row, row);
	usedRows[current_dive]++;
	endInsertRows();
}

void CylindersModel::update()
{
	if (usedRows[current_dive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[current_dive]-1);
		endRemoveRows();
	}
	if (usedRows[current_dive] > 0) {
		beginInsertRows(QModelIndex(), 0, usedRows[current_dive]-1);
		endInsertRows();
	}
}

void CylindersModel::clear()
{
	if (usedRows[current_dive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[current_dive]-1);
		usedRows[current_dive] = 0;
		endRemoveRows();
	}
}

void WeightModel::clear()
{
	if (usedRows[current_dive] > 0) {
		beginRemoveRows(QModelIndex(), 0, usedRows[current_dive]-1);
		usedRows[current_dive] = 0;
		endRemoveRows();
	}
}

int WeightModel::columnCount(const QModelIndex& parent) const
{
	return 2;
}

QVariant WeightModel::data(const QModelIndex& index, int role) const
{
	return QVariant();
}

int WeightModel::rowCount(const QModelIndex& parent) const
{
	return usedRows[current_dive];
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
	DiveItem(): dive(NULL), parentItem(NULL) {}

	DiveItem(struct dive *d, DiveItem *parent = NULL);

	~DiveItem() { qDeleteAll(childlist); }

	int diveNumber() const { return dive->number; }
	const QString diveDateTime() const { return get_dive_date_string(dive->when); }
	int diveDuration() const { return dive->duration.seconds; }
	int diveDepth() const { return dive->maxdepth.mm; }
	int diveSac() const { return dive->sac; }
	int diveOtu() const { return dive->otu; }
	int diveMaxcns() const { return dive->maxcns; }

	int diveWeight() const
	{
		weight_t tw = { total_weight(dive) };
		return tw.grams;
	}

	int diveRating() const { return dive->rating; }

	QString displayDuration() const;
	QString displayDepth() const;
	QString displayTemperature() const;
	QString displayWeight() const;
	QString displaySac() const;
	const QString diveLocation() const { return dive->location; }
	const QString diveSuit() const { return dive->suit; }
	DiveItem *parent() const { return parentItem; }
	const QList<DiveItem *>& children() const { return childlist; }

	void addChild(DiveItem* item) {
		item->parentItem = this;
		childlist.push_back(item);
	} /* parent = self */

private:
	struct dive *dive;
	DiveItem *parentItem;
	QList <DiveItem*> childlist;
};


DiveItem::DiveItem(struct dive *d, DiveItem *p):
	dive(d),
	parentItem(p)
{
	if (parentItem)
		parentItem->addChild(this);
}

QString DiveItem::displayDepth() const
{
	const int scale = 1000;
	QString fract, str;
	if (get_units()->length == units::METERS) {
		fract = QString::number((unsigned)(dive->maxdepth.mm % scale) / 10);
		str = QString("%1.%2").arg((unsigned)(dive->maxdepth.mm / scale)).arg(fract, 2, QChar('0'));
	}
	if (get_units()->length == units::FEET) {
		str = QString::number(mm_to_feet(dive->maxdepth.mm),'f',0);
	}
	return str;
}

QString DiveItem::displayDuration() const
{
	int hrs, mins, secs;

	secs = dive->duration.seconds % 60;
	mins = dive->duration.seconds / 60;
	hrs = mins / 60;
	mins -= hrs * 60;

	QString displayTime;
	if (hrs)
		displayTime = QString("%1:%2:").arg(hrs).arg(mins, 2, 10, QChar('0'));
	else
		displayTime = QString("%1:").arg(mins);
	displayTime += QString("%1").arg(secs, 2, 10, QChar('0'));
	return displayTime;
}

QString DiveItem::displayTemperature() const
{
	QString str;

	if (get_units()->temperature == units::CELSIUS) {
		str = QString::number(mkelvin_to_C(dive->watertemp.mkelvin), 'f', 1);
	} else {
		str = QString::number(mkelvin_to_F(dive->watertemp.mkelvin), 'f', 1);
	}
	return str;
}

QString DiveItem::displaySac() const
{
	QString str;

	if (get_units()->volume == units::LITER) {
		str = QString::number(dive->sac / 1000, 'f', 1);
	} else {
		str = QString::number(ml_to_cuft(dive->sac), 'f', 2);
	}
	return str;
}

QString DiveItem::displayWeight() const
{
	QString str;

	if (get_units()->weight == units::KG) {
		int gr = diveWeight() % 1000;
		int kg = diveWeight() / 1000;
		str = QString("%1.%2").arg(kg).arg((unsigned)(gr + 500) / 100);
	} else {
		str = QString("%1").arg((unsigned)(grams_to_lbs(diveWeight()) + 0.5));
	}
	return str;
}

DiveTripModel::DiveTripModel(QObject *parent) : QAbstractItemModel(parent)
{
	rootItem = new DiveItem;
	int i;
	struct dive *d;

	for_each_dive(i, d) {
		new DiveItem(d, rootItem);
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
	if (role == Qt::TextAlignmentRole) {
		switch (index.column()) {
			case DATE:     /* fall through */
			case SUIT:     /* fall through */
			case LOCATION:
				retVal = Qt::AlignLeft;
				break;
			default:
				retVal = Qt::AlignRight;
				break;
		}
	}
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
			case NR:
				retVal = item->diveNumber();
				break;
			case DATE:
				retVal = item->diveDateTime();
				break;
			case DEPTH:
				retVal = item->displayDepth();
				break;
			case DURATION:
				retVal = item->displayDuration();
				break;
			case TEMPERATURE:
				retVal = item->displayTemperature();
				break;
			case TOTALWEIGHT:
				retVal = item->displayWeight();
				break;
			case SUIT:
				retVal = item->diveSuit();
				break;
			case SAC:
				retVal = item->displaySac();
				break;
			case OTU:
				retVal = item->diveOtu();
				break;
			case MAXCNS:
				retVal = item->diveMaxcns();
				break;
			case LOCATION:
				retVal = item->diveLocation();
				break;
			case RATING:
				retVal = item->diveRating();
				break;
		}
	}
	return retVal;
}


QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation != Qt::Horizontal)
		return ret;

	if (role == Qt::DisplayRole) {
		switch(section) {
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
				if (get_units()->length == units::METERS)
					ret = tr("m");
				else
					ret = tr("ft");
				break;
			case DURATION:
				ret = tr("min");
				break;
			case TEMPERATURE:
				if (get_units()->temperature == units::CELSIUS)
					ret = QString("%1%2").arg(UTF8_DEGREE).arg("C");
				else
					ret = QString("%1%2").arg(UTF8_DEGREE).arg("F");
				break;
			case TOTALWEIGHT:
				if (get_units()->weight == units::KG)
					ret = tr("kg");
				else
					ret = tr("lbs");
				break;
			case SUIT:
				ret = tr("Suit");
				break;
			case CYLINDER:
				ret = tr("Cyl");
				break;
			case NITROX:
				ret = QString("O%1%").arg(UTF8_SUBSCRIPT_2);
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
	if (parent.isValid() && parent.column() > 0)
		return 0;
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
	if (DiveItem *item = parentItem->children().at(row))
		return createIndex(row, column, item);
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
