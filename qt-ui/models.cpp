/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include <QCoreApplication>
#include <QDebug>
#include <QColor>
#include <QBrush>

extern struct tank_info tank_info[100];

CylindersModel::CylindersModel(QObject* parent): QAbstractTableModel(parent)
{
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical)
		return ret;

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
	if (!index.isValid() || index.row() >= MAX_CYLINDERS)
		return ret;

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
		return;
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
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_WEIGHTSYSTEMS)
		return ret;

	weightsystem_t *ws = &current_dive->weightsystem[index.row()];

	if (role == Qt::DisplayRole) {
		switch(index.column()) {
		case TYPE:
			ret = QString(ws->description);
			break;
		case WEIGHT:
			if (get_units()->weight == units::KG) {
				int gr = ws->weight.grams % 1000;
				int kg = ws->weight.grams / 1000;
				ret = QString("%1.%2").arg(kg).arg((unsigned) gr / 100);
			} else {
				ret = QString("%1").arg((unsigned)(grams_to_lbs(ws->weight.grams)));
			}
			break;
		}
	}
	return ret;
}

int WeightModel::rowCount(const QModelIndex& parent) const
{
	return usedRows[current_dive];
}

QVariant WeightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical)
		return ret;

	if (role == Qt::DisplayRole) {
		switch(section) {
		case TYPE:
			ret = tr("Type");
			break;
		case WEIGHT:
			ret = tr("Weight");
			break;
		}
	}
	return ret;
}

void WeightModel::add(weightsystem_t* weight)
{
	if (usedRows[current_dive] >= MAX_WEIGHTSYSTEMS) {
		free(weight);
		return;
	}

	int row = usedRows[current_dive];

	weightsystem_t *ws = &current_dive->weightsystem[row];

	ws->description = weight->description;
	ws->weight.grams = weight->weight.grams;

	beginInsertRows(QModelIndex(), row, row);
	usedRows[current_dive]++;
	endInsertRows();
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
			case BAR:
				ret = bar;
				break;
			case ML:
				ret = ml;
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
	for (info = tank_info; info->name; info++, rows++);

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
	for (info = tank_info; info->name; info++, rows++);

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

TreeItemDT::~TreeItemDT()
{
	qDeleteAll(children);
}

int TreeItemDT::row() const
{
	if (parent)
		return parent->children.indexOf(const_cast<TreeItemDT*>(this));

	return 0;
}

QVariant TreeItemDT::data(int column, int role) const
{
	QVariant ret;
	switch (column) {
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
		ret = (get_units()->length == units::METERS) ? tr("m") : tr("ft");
		break;
	case DURATION:
		ret = tr("min");
		break;
	case TEMPERATURE:
		ret = QString("%1%2").arg(UTF8_DEGREE).arg( (get_units()->temperature == units::CELSIUS) ? "C" : "F");
		break;
	case TOTALWEIGHT:
		ret = (get_units()->weight == units::KG) ? tr("kg") : tr("lbs");
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
	return ret;
}

struct TripItem : public TreeItemDT {
	virtual QVariant data(int column, int role) const;
	dive_trip_t* trip;
};

QVariant TripItem::data(int column, int role) const
{
	QVariant ret;

	if (role == Qt::DisplayRole) {
		switch (column) {
		case LOCATION:
			ret = QString(trip->location);
			break;
		case DATE:
			ret = QString(get_trip_date_string(trip->when, trip->nrdives));
			break;
		}
	}

	return ret;
}

struct DiveItem : public TreeItemDT {
	virtual QVariant data(int column, int role) const;
	struct dive* dive;

	QString displayDuration() const;
	QString displayDepth() const;
	QString displayTemperature() const;
	QString displayWeight() const;
	QString displaySac() const;
	int weight() const;
};

QVariant DiveItem::data(int column, int role) const
{
	QVariant retVal;

	switch (role) {
	case Qt::TextAlignmentRole:
		switch (column) {
		case DATE: /* fall through */
		case SUIT: /* fall through */
		case LOCATION:
			retVal = Qt::AlignLeft;
			break;
		default:
			retVal = Qt::AlignRight;
			break;
		}
		break;
	case Qt::DisplayRole:
		switch (column) {
		case NR:
			retVal = dive->number;
			break;
		case DATE:
			retVal = QString(get_dive_date_string(dive->when));
			break;
		case DEPTH:
			retVal = displayDepth();
			break;
		case DURATION:
			retVal = displayDuration();
			break;
		case TEMPERATURE:
			retVal = displayTemperature();
			break;
		case TOTALWEIGHT:
			retVal = displayWeight();
			break;
		case SUIT:
			retVal = QString(dive->suit);
			break;
		case CYLINDER:
			retVal = QString(dive->cylinder[0].type.description);
			break;
		case NITROX:
			retVal = QString(get_nitrox_string(dive));
			break;
		case SAC:
			retVal = displaySac();
			break;
		case OTU:
			retVal = dive->otu;
			break;
		case MAXCNS:
			retVal = dive->maxcns;
			break;
		case LOCATION:
			retVal = QString(dive->location);
			break;
		}
		break;
	}

	if(role == STAR_ROLE){
		retVal = dive->rating;
	}

	if(role == DIVE_ROLE){
		retVal = QVariant::fromValue<void*>(dive);
	}

	return retVal;
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

	if (!dive->watertemp.mkelvin)
		return str;

	if (get_units()->temperature == units::CELSIUS)
		str = QString::number(mkelvin_to_C(dive->watertemp.mkelvin), 'f', 1);
	else
		str = QString::number(mkelvin_to_F(dive->watertemp.mkelvin), 'f', 1);

	return str;
}

QString DiveItem::displaySac() const
{
	QString str;

	if (get_units()->volume == units::LITER)
		str = QString::number(dive->sac / 1000, 'f', 1);
	else
		str = QString::number(ml_to_cuft(dive->sac), 'f', 2);

	return str;
}

QString DiveItem::displayWeight() const
{
	QString str;

	if (get_units()->weight == units::KG) {
		int gr = weight() % 1000;
		int kg = weight() / 1000;
		str = QString("%1.%2").arg(kg).arg((unsigned)(gr) / 100);
	} else {
		str = QString("%1").arg((unsigned)(grams_to_lbs(weight())));
	}

	return str;
}

int DiveItem::weight() const
{
	weight_t tw = { total_weight(dive) };
	return tw.grams;
}


DiveTripModel::DiveTripModel(QObject* parent) :
	QAbstractItemModel(parent)
{
	rootItem = new TreeItemDT();
	setupModelData();
}

DiveTripModel::~DiveTripModel()
{
	delete rootItem;
}

int DiveTripModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return static_cast<TreeItemDT*>(parent.internalPointer())->columnCount();
	else
		return rootItem->columnCount();
}

QVariant DiveTripModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	TreeItemDT* item = static_cast<TreeItemDT*>(index.internalPointer());

	return item->data(index.column(), role);
}

Qt::ItemFlags DiveTripModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section, role);

	return QVariant();
}

QModelIndex DiveTripModel::index(int row, int column, const QModelIndex& parent)
const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItemDT* parentItem = (!parent.isValid()) ? rootItem : static_cast<TreeItemDT*>(parent.internalPointer());

	TreeItemDT* childItem = parentItem->children[row];

	return (childItem) ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex DiveTripModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItemDT* childItem = static_cast<TreeItemDT*>(index.internalPointer());
	TreeItemDT* parentItem = childItem->parent;

	if (parentItem == rootItem || !parentItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int DiveTripModel::rowCount(const QModelIndex& parent) const
{
	TreeItemDT* parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItemDT*>(parent.internalPointer());

	int amount = parentItem->children.count();

	return amount;
}

void DiveTripModel::setupModelData()
{
	int i = dive_table.nr;

	while (--i >= 0) {
		struct dive* dive = get_dive(i);
		update_cylinder_related_info(dive);
		dive_trip_t* trip = dive->divetrip;

		DiveItem* diveItem = new DiveItem();
		diveItem->dive = dive;

		if (!trip) {
			diveItem->parent = rootItem;
			rootItem->children.push_back(diveItem);
			continue;
		}
		if (!trips.keys().contains(trip)) {
			TripItem* tripItem  = new TripItem();
			tripItem->trip = trip;
			tripItem->parent = rootItem;
			tripItem->children.push_back(diveItem);
			trips[trip] = tripItem;
			rootItem->children.push_back(tripItem);
			continue;
		}
		TripItem* tripItem  = trips[trip];
		tripItem->children.push_back(diveItem);
	}
}
