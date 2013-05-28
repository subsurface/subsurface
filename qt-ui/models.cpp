/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include "../helpers.h"
#include "../dive.h"
#include <QCoreApplication>
#include <QDebug>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QIcon>

CylindersModel::CylindersModel(QObject* parent): QAbstractTableModel(parent), current(0), rows(0)
{
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	QFont font;

	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::FontRole:
		font.setPointSizeF(font.pointSizeF() * 0.8);
		return font;
	case Qt::DisplayRole:
		switch(section) {
		case TYPE:
			ret = tr("Type");
			break;
		case SIZE:
			ret = tr("Size");
			break;
		case WORKINGPRESS:
			ret = tr("WorkPress");
			break;
		case START:
			ret = tr("StartPress");
			break;
		case END:
			ret = tr("EndPress  ");
			break;
		case O2:
			ret = tr("O2% ");
			break;
		case HE:
			ret = tr("He% ");
			break;
		}
	}
	return ret;
}

int CylindersModel::columnCount(const QModelIndex& parent) const
{
	return COLUMNS;
}

QVariant CylindersModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	QFont font;

	if (!index.isValid() || index.row() >= MAX_CYLINDERS)
		return ret;

	cylinder_t *cyl = &current->cylinder[index.row()];
	switch (role) {
	case Qt::FontRole:
		font.setPointSizeF(font.pointSizeF() * 0.80);
		ret = font;
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignRight;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch(index.column()) {
		case TYPE:
			ret = QString(cyl->type.description);
			break;
		case SIZE:
			// we can't use get_volume_string because the idiotic imperial tank
			// sizes take working pressure into account...
			if (cyl->type.size.mliter) {
				if (prefs.units.volume == prefs.units.CUFT) {
					int cuft = 0.5 + ml_to_cuft(gas_volume(cyl, cyl->type.workingpressure));
					ret = QString("%1cuft").arg(cuft);
				} else {
					ret = QString("%1l").arg(cyl->type.size.mliter / 1000.0, 0, 'f', 1);
				}
			}
			break;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar)
				ret = get_pressure_string(cyl->type.workingpressure, TRUE);
			break;
		case START:
			if (cyl->start.mbar)
				ret = get_pressure_string(cyl->start, TRUE);
			break;
		case END:
			if (cyl->end.mbar)
				ret = get_pressure_string(cyl->end, TRUE	);
			break;
		case O2:
			ret = QString("%1%").arg((cyl->gasmix.o2.permille + 5) / 10);
			break;
		case HE:
			ret = QString("%1%").arg((cyl->gasmix.he.permille + 5) / 10);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = QIcon(":trash");
		break;
	}
	return ret;
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void CylindersModel::passInData(const QModelIndex& index, const QVariant& value)
{
	cylinder_t *cyl = &current->cylinder[index.row()];
	switch(index.column()) {
	case SIZE:
		if (cyl->type.size.mliter != value.toInt()) {
			cyl->type.size.mliter = value.toInt();
			mark_divelist_changed(TRUE);
		}
		break;
	case WORKINGPRESS:
		if (cyl->type.workingpressure.mbar != value.toInt()) {
			cyl->type.workingpressure.mbar = value.toInt();
			mark_divelist_changed(TRUE);
		}
		break;
	}
}

#define CHANGED(_t,_u1,_u2) value._t() != data(index, role).toString().replace(_u1,"").replace(_u2,"")._t()

bool CylindersModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	cylinder_t *cyl = &current->cylinder[index.row()];
	switch(index.column()) {
	case TYPE:
		if (!value.isNull()) {
			QByteArray ba = value.toByteArray();
			const char *text = ba.constData();
			if (!cyl->type.description || strcmp(cyl->type.description, text)) {
				cyl->type.description = strdup(text);
				mark_divelist_changed(TRUE);
			}
		}
		break;
	case SIZE:
		if (CHANGED(toDouble, "cuft", "l")) {
			// if units are CUFT then this value is meaningless until we have working pressure
			if (value.toDouble() != 0.0) {
				TankInfoModel *tanks = TankInfoModel::instance();
				QModelIndexList matches = tanks->match(tanks->index(0,0), Qt::DisplayRole, cyl->type.description);
				if (prefs.units.volume == prefs.units.CUFT) {
					if (cyl->type.workingpressure.mbar == 0) {
						// this is a hack as we can't store a wet size
						// without working pressure in cuft mode
						// so we assume it's an aluminum tank at 3000psi
						cyl->type.workingpressure.mbar = psi_to_mbar(3000);
						if (!matches.isEmpty())
							tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
					}
					if (cyl->type.size.mliter != wet_volume(value.toDouble(), cyl->type.workingpressure)) {
						mark_divelist_changed(TRUE);
						cyl->type.size.mliter = wet_volume(value.toDouble(), cyl->type.workingpressure);
						if (!matches.isEmpty())
							tanks->setData(tanks->index(matches.first().row(), TankInfoModel::ML), cyl->type.size.mliter);
					}
				} else {
					if (cyl->type.size.mliter != value.toDouble() * 1000.0) {
						mark_divelist_changed(TRUE);
						cyl->type.size.mliter = value.toDouble() * 1000.0;
						if (!matches.isEmpty())
							tanks->setData(tanks->index(matches.first().row(), TankInfoModel::ML), cyl->type.size.mliter);
					}
				}
			}
		}
		break;
	case WORKINGPRESS:
		if (CHANGED(toDouble, "psi", "bar")) {
			if (value.toDouble() != 0.0) {
				TankInfoModel *tanks = TankInfoModel::instance();
				QModelIndexList matches = tanks->match(tanks->index(0,0), Qt::DisplayRole, cyl->type.description);
				if (prefs.units.pressure == prefs.units.PSI)
					cyl->type.workingpressure.mbar = psi_to_mbar(value.toDouble());
				else
					cyl->type.workingpressure.mbar = value.toDouble() * 1000;
				if (!matches.isEmpty())
					tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
				mark_divelist_changed(TRUE);
			}
		}
		break;
	case START:
		if (CHANGED(toDouble, "psi", "bar")) {
			if (value.toDouble() != 0.0) {
				if (prefs.units.pressure == prefs.units.PSI)
					cyl->start.mbar = psi_to_mbar(value.toDouble());
				else
					cyl->start.mbar = value.toDouble() * 1000;
				mark_divelist_changed(TRUE);
			}
		}
		break;
	case END:
		if (CHANGED(toDouble, "psi", "bar")) {
			if (value.toDouble() != 0.0) {
				if (prefs.units.pressure == prefs.units.PSI)
					cyl->end.mbar = psi_to_mbar(value.toDouble());
				else
					cyl->end.mbar = value.toDouble() * 1000;
			}
		}
		break;
	case O2:
		if (CHANGED(toInt, "%", "%")) {
			cyl->gasmix.o2.permille = value.toInt() * 10 - 5;
			mark_divelist_changed(TRUE);
		}
		break;
	case HE:
		if (CHANGED(toInt, "%", "%")) {
			cyl->gasmix.he.permille = value.toInt() * 10 - 5;
			mark_divelist_changed(TRUE);
		}
		break;
	}
	return QAbstractItemModel::setData(index, value, role);
}

int CylindersModel::rowCount(const QModelIndex& parent) const
{
	return 	rows;
}

void CylindersModel::add()
{
	if (rows >= MAX_CYLINDERS) {
		return;
	}

	int row = rows;

	beginInsertRows(QModelIndex(), row, row);
	rows++;
	endInsertRows();
}

void CylindersModel::update()
{
	setDive(current);
}

void CylindersModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows-1);
		endRemoveRows();
	}
}

void CylindersModel::setDive(dive* d)
{
	if (current)
		clear();

	int amount = MAX_CYLINDERS;
	for(int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cylinder = &d->cylinder[i];
		if (cylinder_none(cylinder)) {
			amount = i;
			break;
		}
	}

	beginInsertRows(QModelIndex(), 0, amount-1);
	rows = amount;
	current = d;
	endInsertRows();
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex& index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void CylindersModel::remove(const QModelIndex& index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_cylinder(current, index.row());
	mark_divelist_changed(TRUE);
	endRemoveRows();
}

WeightModel::WeightModel(QObject* parent): QAbstractTableModel(parent), current(0), rows(0)
{
}

void WeightModel::remove(const QModelIndex& index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_weightsystem(current, index.row());
	mark_divelist_changed(TRUE);
	endRemoveRows();
}

void WeightModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows-1);
		endRemoveRows();
	}
}

int WeightModel::columnCount(const QModelIndex& parent) const
{
	return COLUMNS;
}

QVariant WeightModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	QFont font;
	if (!index.isValid() || index.row() >= MAX_WEIGHTSYSTEMS)
		return ret;

	weightsystem_t *ws = &current->weightsystem[index.row()];

	switch (role) {
	case Qt::FontRole:
		font.setPointSizeF(font.pointSizeF() * 0.80);
		ret = font;
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignRight;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch(index.column()) {
		case TYPE:
			ret = QString(ws->description);
			break;
		case WEIGHT:
			ret = get_weight_string(ws->weight, TRUE);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = QIcon(":trash");
		break;
	}
	return ret;
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void WeightModel::passInData(const QModelIndex& index, const QVariant& value)
{
	weightsystem_t *ws = &current->weightsystem[index.row()];
	if (index.column() == WEIGHT) {
		if (ws->weight.grams != value.toInt()) {
			ws->weight.grams = value.toInt();
			mark_divelist_changed(TRUE);
		}
	}
}

bool WeightModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	weightsystem_t *ws = &current->weightsystem[index.row()];
	switch(index.column()) {
	case TYPE:
		if (!value.isNull()) {
			QByteArray ba = value.toString().toUtf8();
			const char *text = ba.constData();
			if (!ws->description || strcmp(ws->description, text)) {
				ws->description = strdup(text);
				mark_divelist_changed(TRUE);
			}
		}
		break;
	case WEIGHT:
		if (CHANGED(toDouble, "kg", "lbs")) {
			if (prefs.units.weight == prefs.units.LBS)
				ws->weight.grams = lbs_to_grams(value.toDouble());
			else
				ws->weight.grams = value.toDouble() * 1000.0;
			// now update the ws_info
			WSInfoModel *wsim = WSInfoModel::instance();
			QModelIndexList matches = wsim->match(wsim->index(0,0), Qt::DisplayRole, ws->description);
			if (!matches.isEmpty())
				wsim->setData(wsim->index(matches.first().row(), WSInfoModel::GR), ws->weight.grams);
		}
		break;
	}
	return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags WeightModel::flags(const QModelIndex& index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int WeightModel::rowCount(const QModelIndex& parent) const
{
	return rows;
}

QVariant WeightModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	QFont font;
	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::FontRole:
		font.setPointSizeF(font.pointSizeF() * 0.8);
		ret = font;
		break;
	case Qt::DisplayRole:
		switch(section) {
		case TYPE:
			ret = tr("Type");
			break;
		case WEIGHT:
			ret = tr("Weight");
			break;
		}
		break;
	}
	return ret;
}

void WeightModel::add()
{
	if (rows >= MAX_WEIGHTSYSTEMS)
		return;

	int row = rows;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	endInsertRows();
}

void WeightModel::update()
{
	setDive(current);
}

void WeightModel::setDive(dive* d)
{
	if (current)
		clear();

	int amount = MAX_WEIGHTSYSTEMS;
	for(int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		weightsystem_t *weightsystem = &d->weightsystem[i];
		if (weightsystem_none(weightsystem)) {
			amount = i;
			break;
		}
	}

	beginInsertRows(QModelIndex(), 0, amount-1);
	rows = amount;
	current = d;
	endInsertRows();
}

WSInfoModel* WSInfoModel::instance()
{
	static WSInfoModel *self = new WSInfoModel();
	return self;
}

bool WSInfoModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool WSInfoModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	struct ws_info *info = &ws_info[index.row()];
	switch(index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case GR:
		info->grams = value.toInt();
		break;
	}
	return TRUE;
}

void WSInfoModel::clear()
{
}

int WSInfoModel::columnCount(const QModelIndex& parent) const
{
	return 2;
}

QVariant WSInfoModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct ws_info *info = &ws_info[index.row()];

	int gr = info->grams;

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch(index.column()) {
			case GR:
				ret = gr;
				break;
			case DESCRIPTION:
				ret = QString(info->name);
				break;
		}
	}
	return ret;
}

QVariant WSInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;

	if (orientation != Qt::Horizontal)
		return ret;

	if (role == Qt::DisplayRole) {
		switch(section) {
			case GR:
				ret = tr("kg");
				break;
			case DESCRIPTION:
				ret = tr("Description");
				break;
		}
	}
	return ret;
}

int WSInfoModel::rowCount(const QModelIndex& parent) const
{
	return rows+1;
}

WSInfoModel::WSInfoModel() : QAbstractTableModel(), rows(-1)
{
	struct ws_info *info = ws_info;
	for (info = ws_info; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::update()
{
	if (rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
		rows = -1;
	}
	struct ws_info *info = ws_info;
	for (info = ws_info; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

TankInfoModel* TankInfoModel::instance()
{
	static TankInfoModel *self = new TankInfoModel();
	return self;
}

bool TankInfoModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool TankInfoModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	struct tank_info *info = &tank_info[index.row()];
	switch(index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case ML:
		info->ml = value.toInt();
		break;
	case BAR:
		info->bar = value.toInt();
		break;
	}
	return TRUE;
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

	if (info->cuft && info->psi) {
		pressure_t p;
		p.mbar = psi_to_mbar(info->psi);
		ml = wet_volume(info->cuft, p);
	}
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
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
	if (rows > -1) {
		beginRemoveRows(QModelIndex(), 0, rows);
		endRemoveRows();
		rows = -1;
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
		ret = QString("%1%2").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
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

	if (role == STAR_ROLE)
		retVal = dive->rating;

	if (role == DIVE_ROLE)
		retVal = QVariant::fromValue<void*>(dive);

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

	if (role == Qt::FontRole) {
		QFont font;
		font.setPointSizeF(font.pointSizeF() * 0.7);
		return font;
	}
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

	if (rowCount()){
		beginRemoveRows(QModelIndex(), 0, rowCount()-1);
		endRemoveRows();
	}

	while (--i >= 0) {
		struct dive* dive = get_dive(i);
		update_cylinder_related_info(dive);
		dive_trip_t* trip = dive->divetrip;

		DiveItem* diveItem = new DiveItem();
		diveItem->dive = dive;

		if (!trip || currentLayout == LIST) {
			diveItem->parent = rootItem;
			rootItem->children.push_back(diveItem);
			continue;
		}
		if (currentLayout == LIST)
			continue;

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

	if (rowCount()){
		beginInsertRows(QModelIndex(), 0, rowCount()-1);
		endInsertRows();
	}
}

DiveTripModel::Layout DiveTripModel::layout() const
{
	return currentLayout;
}

void DiveTripModel::setLayout(DiveTripModel::Layout layout)
{
	currentLayout = layout;
	setupModelData();
}
