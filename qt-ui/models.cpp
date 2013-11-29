/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include "diveplanner.h"
#include "mainwindow.h"
#include "../helpers.h"
#include "../dive.h"
#include "../device.h"
#include "../statistics.h"
#include "../qthelper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QIcon>
#include <QMessageBox>

QFont defaultModelFont()
{
	QFont font;
	font.setPointSizeF( font.pointSizeF() * 0.8);
	return font;
}

CleanerTableModel::CleanerTableModel(): QAbstractTableModel()
{
}

int CleanerTableModel::columnCount(const QModelIndex& parent) const
{
	return headers.count();
}

QVariant CleanerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;

	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
		ret = headers.at(section);
	}
	return ret;
}

void CleanerTableModel::setHeaderDataStrings(const QStringList& newHeaders)
{
	headers = newHeaders;
}

CylindersModel::CylindersModel(QObject* parent): current(0), rows(0)
{
	//	enum{REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, DEPTH};
	setHeaderDataStrings( QStringList() <<  "" << tr("Type") << tr("Size") << tr("WorkPress") << tr("StartPress") << tr("EndPress") <<  tr("O2%") << tr("HE") << tr("Switch at"));
}

CylindersModel *CylindersModel::instance()
{
	static CylindersModel *self = new CylindersModel();
	return self;
}

static QVariant percent_string(fraction_t fraction)
{
	int permille = fraction.permille;

	if (!permille)
		return QVariant();
	return QString("%1%").arg(permille / 10.0, 0, 'f', 1);
}

QVariant CylindersModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;

	if (!index.isValid() || index.row() >= MAX_CYLINDERS)
		return ret;

	cylinder_t *cyl = &current->cylinder[index.row()];
	switch (role) {
	case Qt::FontRole: {
		QFont font = defaultModelFont();
		switch (index.column()) {
		case START: font.setItalic(!cyl->start.mbar); break;
		case END: font.setItalic(!cyl->end.mbar); break;
		}
		ret = font;
		break;
	}
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
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
				double volume;
				int mbar = cyl->type.workingpressure.mbar;

				if (mbar && prefs.units.volume == prefs.units.CUFT) {
					volume = ml_to_cuft(cyl->type.size.mliter);
					volume *= bar_to_atm(mbar / 1000.0);
				} else {
					volume = cyl->type.size.mliter / 1000.0;
				}
				ret = QString("%1").arg(volume, 0, 'f', 1);
			}
			break;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar)
				ret = get_pressure_string(cyl->type.workingpressure, TRUE);
			break;
		case START:
			if (cyl->start.mbar)
				ret = get_pressure_string(cyl->start, FALSE);
			else if (cyl->sample_start.mbar)
				ret = get_pressure_string(cyl->sample_start, FALSE);
			break;
		case END:
			if (cyl->end.mbar)
				ret = get_pressure_string(cyl->end, FALSE);
			else if (cyl->sample_end.mbar)
				ret = get_pressure_string(cyl->sample_end, FALSE);
			break;
		case O2:
			ret = percent_string(cyl->gasmix.o2);
			break;
		case HE:
			ret = percent_string(cyl->gasmix.he);
			break;
		case DEPTH:
			if (prefs.units.length == prefs.units.FEET)
				ret = mm_to_feet(cyl->depth.mm);
			else
				ret = cyl->depth.mm / 1000;
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = QIcon(":trash");
		break;

	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this cylinder.");
		break;
	}

	return ret;
}

cylinder_t* CylindersModel::cylinderAt(const QModelIndex& index)
{
	return &current->cylinder[index.row()];
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void CylindersModel::passInData(const QModelIndex& index, const QVariant& value)
{
	cylinder_t *cyl = cylinderAt(index);
	switch(index.column()) {
	case SIZE:
		if (cyl->type.size.mliter != value.toInt()) {
			cyl->type.size.mliter = value.toInt();
			dataChanged(index, index);
		}
		break;
	case WORKINGPRESS:
		if (cyl->type.workingpressure.mbar != value.toInt()) {
			cyl->type.workingpressure.mbar = value.toInt();
			dataChanged(index, index);
		}
		break;
	}
}

#define CHANGED(_t,_u1,_u2) \
	value.toString().remove(_u1).remove(_u2)._t() !=  \
	data(index, role).toString().remove(_u1).remove(_u2)._t()

bool CylindersModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	bool addDiveMode = DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING;
	if (addDiveMode)
		DivePlannerPointsModel::instance()->rememberTanks();

	cylinder_t *cyl = cylinderAt(index);
	switch(index.column()) {
	case TYPE:
		if (!value.isNull()) {
			QByteArray ba = value.toByteArray();
			const char *text = ba.constData();
			if (!cyl->type.description || strcmp(cyl->type.description, text)) {
				cyl->type.description = strdup(text);
				changed = true;
			}
		}
		break;
	case SIZE:
		if (CHANGED(toDouble, "cuft", "l")) {
			// if units are CUFT then this value is meaningless until we have working pressure
			if (value.toDouble() != 0.0) {
				TankInfoModel *tanks = TankInfoModel::instance();
				QModelIndexList matches = tanks->match(tanks->index(0,0), Qt::DisplayRole, cyl->type.description);
				int mbar = cyl->type.workingpressure.mbar;
				int mliter;

				if (mbar && prefs.units.volume == prefs.units.CUFT) {
					double liters = cuft_to_l(value.toDouble());
					liters /= bar_to_atm(mbar / 1000.0);
					mliter = rint(liters * 1000);
				} else {
					mliter = rint(value.toDouble() * 1000);
				}
				if (cyl->type.size.mliter != mliter) {
					mark_divelist_changed(TRUE);
					cyl->type.size.mliter = mliter;
					if (!matches.isEmpty())
						tanks->setData(tanks->index(matches.first().row(), TankInfoModel::ML), cyl->type.size.mliter);
				}
				changed = true;
			}
		}
		break;
	case WORKINGPRESS:
		if (CHANGED(toDouble, "psi", "bar")) {
			QString vString = value.toString();
			vString.remove("psi").remove("bar");
			if (vString.toDouble() != 0.0) {
				TankInfoModel *tanks = TankInfoModel::instance();
				QModelIndexList matches = tanks->match(tanks->index(0,0), Qt::DisplayRole, cyl->type.description);
				if (prefs.units.pressure == prefs.units.PSI)
					cyl->type.workingpressure.mbar = psi_to_mbar(vString.toDouble());
				else
					cyl->type.workingpressure.mbar = vString.toDouble() * 1000;
				if (!matches.isEmpty())
					tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
				changed = true;
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
				changed = true;
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
				changed = true;
			}
		}
		break;
	case O2:
		if (CHANGED(toDouble, "%", "%")) {
			int o2 = value.toString().remove('%').toDouble() * 10 + 0.5;
			if (cyl->gasmix.he.permille + o2 <= 1000) {
				cyl->gasmix.o2.permille = o2;
				changed = true;
			}
		}
		break;
	case HE:
		if (CHANGED(toDouble, "%", "%")) {
			int he = value.toString().remove('%').toDouble() * 10 + 0.5;
			if (cyl->gasmix.o2.permille + he <= 1000) {
				cyl->gasmix.he.permille = he;
				changed = true;
			}
		}
		break;
	case DEPTH:
		if (CHANGED(toDouble, "ft", "m")) {
			if (value.toInt() != 0) {
				if (prefs.units.length == prefs.units.FEET)
					cyl->depth.mm = feet_to_mm(value.toString().remove("ft").remove("m").toInt());
				else
					cyl->depth.mm = value.toString().remove("ft").remove("m").toInt() * 1000;
			}
		}
	}
	dataChanged(index, index);
	if (addDiveMode)
		DivePlannerPointsModel::instance()->tanksUpdated();
	return true;
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
	fill_default_cylinder(&current->cylinder[row]);
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
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
	if (!d)
		return;
	rows = 0;
	for(int i = 0; i < MAX_CYLINDERS; i++) {
		if (!cylinder_none(&d->cylinder[i])) {
			rows = i+1;
		}
	}
	current = d;
	changed = false;
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows-1);
		endInsertRows();
	}
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
	cylinder_t *cyl = &current->cylinder[index.row()];
	if (DivePlannerPointsModel::instance()->tankInUse(cyl->gasmix.o2.permille, cyl->gasmix.he.permille)) {
		QMessageBox::warning(mainWindow(), TITLE_OR_TEXT(
				tr("Cylinder cannot be removed"),
				tr("This gas in use. Only cylinders that are not used in the dive can be removed.")),
				QMessageBox::Ok);
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_cylinder(current, index.row());
	changed = true;
	endRemoveRows();
}

WeightModel::WeightModel(QObject* parent): current(0), rows(0)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Weight"));
}

weightsystem_t* WeightModel::weightSystemAt(const QModelIndex& index)
{
	return &current->weightsystem[index.row()];
}

void WeightModel::remove(const QModelIndex& index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_weightsystem(current, index.row());
	changed = true;
	endRemoveRows();
}

void WeightModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows-1);
		endRemoveRows();
	}
}

QVariant WeightModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_WEIGHTSYSTEMS)
		return ret;

	weightsystem_t *ws = &current->weightsystem[index.row()];

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
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
	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this weigthsystem.");
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
			dataChanged(index, index);
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
				changed = true;
			}
		}
		break;
	case WEIGHT:
		if (CHANGED(toDouble, "kg", "lbs")) {
			if (prefs.units.weight == prefs.units.LBS)
				ws->weight.grams = lbs_to_grams(value.toDouble());
			else
				ws->weight.grams = value.toDouble() * 1000.0 + 0.5;
			// now update the ws_info
			changed = true;
			WSInfoModel *wsim = WSInfoModel::instance();
			QModelIndexList matches = wsim->match(wsim->index(0,0), Qt::DisplayRole, ws->description);
			if (!matches.isEmpty())
				wsim->setData(wsim->index(matches.first().row(), WSInfoModel::GR), ws->weight.grams);
		}
		break;
	}
	dataChanged(index, index);
	return true;
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

void WeightModel::add()
{
	if (rows >= MAX_WEIGHTSYSTEMS)
		return;

	int row = rows;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
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
	rows = 0;
	for(int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		if (!weightsystem_none(&d->weightsystem[i])) {
			rows = i+1;
		}
	}
	current = d;
	changed = false;
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows-1);
		endInsertRows();
	}
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
	struct ws_info_t *info = &ws_info[index.row()];
	switch(index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case GR:
		info->grams = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return TRUE;
}

void WSInfoModel::clear()
{
}

QVariant WSInfoModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct ws_info_t *info = &ws_info[index.row()];

	int gr = info->grams;
	switch(role){
		case Qt::FontRole :
			ret = defaultModelFont();
			break;
		case Qt::DisplayRole :
		case Qt::EditRole :
			switch(index.column()) {
				case GR:
					ret = gr;
					break;
				case DESCRIPTION:
					ret = QString(info->name);
					break;
			}
			break;
	}
	return ret;
}

int WSInfoModel::rowCount(const QModelIndex& parent) const
{
	return rows+1;
}

const QString& WSInfoModel::biggerString() const
{
	return biggerEntry;
}

WSInfoModel::WSInfoModel() : rows(-1)
{
	setHeaderDataStrings( QStringList() << tr("Description") << tr("kg"));
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++){
		QString wsInfoName(info->name);
		if( wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

void WSInfoModel::updateInfo()
{
	struct ws_info_t *info = ws_info;
	beginRemoveRows(QModelIndex(), 0, this->rows);
	endRemoveRows();
	rows = -1;
	for (info = ws_info; info->name; info++, rows++){
		QString wsInfoName(info->name);
		if( wsInfoName.count() > biggerEntry.count())
			biggerEntry = wsInfoName;
	}

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
	struct ws_info_t *info = ws_info;
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

const QString& TankInfoModel::biggerString() const
{
	return biggerEntry;
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
	struct tank_info_t *info = &tank_info[index.row()];
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
	emit dataChanged(index, index);
	return TRUE;
}

void TankInfoModel::clear()
{
}

QVariant TankInfoModel::data(const QModelIndex& index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	if (role == Qt::FontRole){
		return defaultModelFont();
	}
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		struct tank_info_t *info = &tank_info[index.row()];
		int ml = info->ml;
		double bar = (info->psi) ? psi_to_bar(info->psi) : info->bar;

		if (info->cuft && info->psi)
			ml = cuft_to_l(info->cuft) * 1000 / bar_to_atm(bar);

		switch(index.column()) {
			case BAR:
				ret = bar * 1000;
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

int TankInfoModel::rowCount(const QModelIndex& parent) const
{
	return rows+1;
}

TankInfoModel::TankInfoModel() :  rows(-1)
{
	setHeaderDataStrings( QStringList() << tr("Description") << tr("ml") << tr("bar"));
	struct tank_info_t *info = tank_info;
	for (info = tank_info; info->name; info++, rows++){
		QString infoName(info->name);
		if (infoName.count() > biggerEntry.count())
			biggerEntry = infoName;
	}

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
	struct tank_info_t *info = tank_info;
	for (info = tank_info; info->name; info++, rows++);

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

//#################################################################################################
//#
//#	Tree Model - a Basic Tree Model so I don't need to kill myself repeating this for every model.
//#
//#################################################################################################

/*! A DiveItem for use with a DiveTripModel
 *
 * A simple class which wraps basic stats for a dive (e.g. duration, depth) and
 * tidies up after it's children. This is done manually as we don't inherit from
 * QObject.
 *
*/

TreeItem::TreeItem()
{
	parent = NULL;
}

TreeItem::~TreeItem()
{
	qDeleteAll(children);
}

Qt::ItemFlags TreeItem::flags(const QModelIndex& index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TreeItem::row() const
{
	if (parent)
		return parent->children.indexOf(const_cast<TreeItem*>(this));
	return 0;
}

QVariant TreeItem::data(int column, int role) const
{
	return QVariant();
}

TreeModel::TreeModel(QObject* parent): QAbstractItemModel(parent)
{
	rootItem = new TreeItem();
}

TreeModel::~TreeModel()
{
	delete rootItem;
}

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	QVariant val = item->data(index.column(), role);

	if (role == Qt::FontRole && !val.isValid())
		return defaultModelFont();
	else
		return val;
}

bool TreeItem::setData(const QModelIndex& index, const QVariant& value, int role)
{
	return false;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent)
const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem* parentItem = (!parent.isValid()) ? rootItem : static_cast<TreeItem*>(parent.internalPointer());

	TreeItem* childItem = parentItem->children[row];

	return (childItem) ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem* parentItem = childItem->parent;

	if (parentItem == rootItem || !parentItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
	TreeItem* parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem*>(parent.internalPointer());

	int amount = parentItem->children.count();
	return amount;
}

int TreeModel::columnCount(const QModelIndex& parent) const
{
	return columns;
}

/*################################################################
 *
 *  Implementation of the Dive List.
 *
 * ############################################################### */
struct TripItem : public TreeItem {
	virtual QVariant data(int column, int role) const;
	dive_trip_t* trip;
};

QVariant TripItem::data(int column, int role) const
{
	QVariant ret;

	if (role == DiveTripModel::TRIP_ROLE)
		return QVariant::fromValue<void*>(trip);

	if (role == DiveTripModel::SORT_ROLE)
		return (qulonglong)trip->when;

	if (role == Qt::DisplayRole) {
		switch (column) {
			case DiveTripModel::NR:
			if (trip->location && *trip->location)
				ret = QString(trip->location) + ", " + get_trip_date_string(trip->when, trip->nrdives);
			else
				ret = get_trip_date_string(trip->when, trip->nrdives);
			break;
		}
	}

	return ret;
}

static int nitrox_sort_value(struct dive *dive)
{
	int o2, he, o2low;
	get_dive_gas(dive, &o2, &he, &o2low);
	return he*1000 + o2;
}

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
		case DiveTripModel::SORT_ROLE:
		switch (column) {
		case NR:		retVal = (qulonglong) dive->when; break;
		case DATE:		retVal = (qulonglong) dive->when; break;
		case RATING:		retVal = dive->rating; break;
		case DEPTH:		retVal = dive->maxdepth.mm; break;
		case DURATION:		retVal = dive->duration.seconds; break;
		case TEMPERATURE:	retVal = dive->watertemp.mkelvin; break;
		case TOTALWEIGHT:	retVal = total_weight(dive); break;
		case SUIT:		retVal = QString(dive->suit); break;
		case CYLINDER:		retVal = QString(dive->cylinder[0].type.description); break;
		case NITROX:		retVal = nitrox_sort_value(dive); break;
		case SAC:		retVal = dive->sac; break;
		case OTU:		retVal = dive->otu; break;
		case MAXCNS:		retVal = dive->maxcns; break;
		case LOCATION:		retVal = QString(dive->location); break;
		}
		break;
	case Qt::DisplayRole:
		switch (column) {
		case NR:		retVal = dive->number; break;
		case DATE:		retVal = displayDate(); break;
		case DEPTH:		retVal = displayDepth(); break;
		case DURATION:		retVal = displayDuration(); break;
		case TEMPERATURE:	retVal = displayTemperature(); break;
		case TOTALWEIGHT:	retVal = displayWeight(); break;
		case SUIT:		retVal = QString(dive->suit); break;
		case CYLINDER:		retVal = QString(dive->cylinder[0].type.description); break;
		case NITROX:		retVal = QString(get_nitrox_string(dive)); break;
		case SAC:		retVal = displaySac(); break;
		case OTU:		retVal = dive->otu; break;
		case MAXCNS:		retVal = dive->maxcns; break;
		case LOCATION:		retVal = QString(dive->location); break;
		}
		break;
	}

	if (role == DiveTripModel::STAR_ROLE)
		retVal = dive->rating;

	if (role == DiveTripModel::DIVE_ROLE)
		retVal = QVariant::fromValue<void*>(dive);

	if(role == DiveTripModel::DIVE_IDX){
		retVal = get_divenr(dive);
	}
	return retVal;
}

Qt::ItemFlags DiveItem::flags(const QModelIndex& index) const
{
	if(index.column() == NR){
		return TreeItem::flags(index) | Qt::ItemIsEditable;
	}
	return TreeItem::flags(index);
}

bool DiveItem::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role != Qt::EditRole)
		return false;
	if (index.column() != NR)
		return false;

	int v = value.toInt();
	if (v == 0)
		return false;

	int i;
	struct dive *d;
	for_each_dive(i, d){
		if (d->number == v)
			return false;
	}

	dive->number = value.toInt();
	mark_divelist_changed(TRUE);
	return true;
}

QString DiveItem::displayDate() const
{
	return get_dive_date_string(dive->when);
}

QString DiveItem::displayDepth() const
{
	const int scale = 1000;
	QString fract, str;
	if (get_units()->length == units::METERS) {
		fract = QString::number((unsigned)(dive->maxdepth.mm % scale) / 100);
		str = QString("%1.%2").arg((unsigned)(dive->maxdepth.mm / scale)).arg(fract, 1, QChar('0'));
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
		str = QString::number(dive->sac / 1000.0, 'f', 1).append(tr(" l/min"));
	else
		str = QString::number(ml_to_cuft(dive->sac), 'f', 2).append(tr(" cuft/min"));
	return str;
}

QString DiveItem::displayWeight() const
{
	QString str = weight_string(weight());
	return str;
}

int DiveItem::weight() const
{
	weight_t tw = { total_weight(dive) };
	return tw.grams;
}

DiveTripModel::DiveTripModel(QObject* parent): TreeModel(parent)
{
	columns = COLUMNS;
}

Qt::ItemFlags DiveTripModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return 0;

	TripItem *item = static_cast<TripItem*>(index.internalPointer());
	return item->flags(index);
}

QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical)
		return ret;

	switch(role){
		case Qt::FontRole :
			ret = defaultModelFont(); break;
		case Qt::DisplayRole :
			switch (section) {
			case NR:		ret = tr("#"); break;
			case DATE:		ret = tr("Date"); break;
			case RATING:	ret = UTF8_BLACKSTAR; break;
			case DEPTH:		ret = (get_units()->length == units::METERS) ? tr("m") : tr("ft"); break;
			case DURATION:	ret = tr("min"); break;
			case TEMPERATURE:ret = QString("%1%2").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F"); break;
			case TOTALWEIGHT:ret = (get_units()->weight == units::KG) ? tr("kg") : tr("lbs"); break;
			case SUIT:		ret = tr("Suit"); break;
			case CYLINDER:	ret = tr("Cyl"); break;
			case NITROX:	ret = QString("O%1%").arg(UTF8_SUBSCRIPT_2); break;
			case SAC:		ret = tr("SAC"); break;
			case OTU:		ret = tr("OTU"); break;
			case MAXCNS:	ret = tr("maxCNS"); break;
			case LOCATION:	ret = tr("Location"); break;
			}break;
	}

	return ret;
}

void DiveTripModel::setupModelData()
{
	int i = dive_table.nr;

	if (rowCount()){
		beginRemoveRows(QModelIndex(), 0, rowCount()-1);
		endRemoveRows();
	}

	if (autogroup)
		autogroup_dives();
	dive_table.preexisting = dive_table.nr;
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
		beginInsertRows(QModelIndex(), 0, rowCount() - 1);
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

bool DiveTripModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	DiveItem *diveItem = dynamic_cast<DiveItem*>(item);
	if(!diveItem)
		return false;
	return diveItem->setData(index, value, role);}


/*####################################################################
 *
 * Dive Computer Model
 *
 *####################################################################
 */

DiveComputerModel::DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject* parent): CleanerTableModel()
{
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
	dcWorkingMap = dcMap;
	numRows = 0;
}

QVariant DiveComputerModel::data(const QModelIndex& index, int role) const
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());

	QVariant ret;
	if (role == Qt::DisplayRole || role == Qt::EditRole){
		switch(index.column()){
			case ID:	ret = QString("0x").append(QString::number(node.deviceId, 16)); break;
			case MODEL:	ret = node.model; break;
			case NICKNAME:	ret = node.nickName; break;
		}
	}

	if (index.column() == REMOVE){
		switch(role){
			case Qt::DecorationRole : ret = QIcon(":trash"); break;
			case Qt::ToolTipRole : ret = tr("Clicking here will remove this divecomputer."); break;
		}
	}
	return ret;
}

int DiveComputerModel::rowCount(const QModelIndex& parent) const
{
	return numRows;
}

void DiveComputerModel::update()
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	int count = values.count();

	if(numRows){
		beginRemoveRows(QModelIndex(), 0, numRows-1);
		numRows = 0;
		endRemoveRows();
	}

	if (count){
		beginInsertRows(QModelIndex(), 0, count-1);
		numRows = count;
		endInsertRows();
	}
}

Qt::ItemFlags DiveComputerModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.column() == NICKNAME)
		flags |= Qt::ItemIsEditable;
    return flags;
}

bool DiveComputerModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());
	dcWorkingMap.remove(node.model, node);
	node.nickName = value.toString();
	dcWorkingMap.insert(node.model, node);
	emit dataChanged(index, index);
	return true;
}

void DiveComputerModel::remove(const QModelIndex& index)
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());
	dcWorkingMap.remove(node.model, node);
	update();
}

void DiveComputerModel::dropWorkingList()
{
	// how do I prevent the memory leak ?
}

void DiveComputerModel::keepWorkingList()
{
	if (dcList.dcMap != dcWorkingMap)
		mark_divelist_changed(TRUE);
	dcList.dcMap = dcWorkingMap;
}

/*#################################################################
 * #
 * #	Yearly Statistics Model
 * #
 * ################################################################
 */

class YearStatisticsItem : public TreeItem{
public:
	enum {YEAR, DIVES, TOTAL_TIME, AVERAGE_TIME, SHORTEST_TIME, LONGEST_TIME, AVG_DEPTH, MIN_DEPTH,
		MAX_DEPTH, AVG_SAC, MIN_SAC, MAX_SAC, AVG_TEMP, MIN_TEMP, MAX_TEMP, COLUMNS};

	QVariant data(int column, int role) const;
	YearStatisticsItem(stats_t interval);
private:
	stats_t stats_interval;
};

YearStatisticsItem::YearStatisticsItem(stats_t interval) : stats_interval(interval)
{
}

QVariant YearStatisticsItem::data(int column, int role) const
{
	double value;
	QVariant ret;

	if (role == Qt::FontRole) {
		QFont font = defaultModelFont();
		font.setBold(stats_interval.is_year);
		return font;
	} else if (role != Qt::DisplayRole) {
		return ret;
	}
	switch(column) {
	case YEAR:
		if (stats_interval.is_trip) {
			ret = stats_interval.location;
		} else {
			ret =  stats_interval.period;
		}
		break;
	case DIVES:		ret =  stats_interval.selection_size; break;
	case TOTAL_TIME:	ret = get_time_string(stats_interval.total_time.seconds, 0); break;
	case AVERAGE_TIME:	ret = get_minutes(stats_interval.total_time.seconds / stats_interval.selection_size); break;
	case SHORTEST_TIME:	ret = get_minutes(stats_interval.shortest_time.seconds); break;
	case LONGEST_TIME:	ret = get_minutes(stats_interval.longest_time.seconds); break;
	case AVG_DEPTH:		ret = get_depth_string(stats_interval.avg_depth); break;
	case MIN_DEPTH:		ret = get_depth_string(stats_interval.min_depth); break;
	case MAX_DEPTH:		ret = get_depth_string(stats_interval.max_depth); break;
	case AVG_SAC:		ret = get_volume_string(stats_interval.avg_sac); break;
	case MIN_SAC:		ret = get_volume_string(stats_interval.min_sac); break;
	case MAX_SAC:		ret = get_volume_string(stats_interval.max_sac); break;
	case AVG_TEMP:
		if (stats_interval.combined_temp && stats_interval.combined_count) {
			ret = QString::number(stats_interval.combined_temp / stats_interval.combined_count, 'f', 1);
		}
		break;
	case MIN_TEMP:
		value = get_temp_units(stats_interval.min_temp, NULL);
		if (value > -100.0)
			ret =  QString::number(value, 'f', 1);
		break;
	case MAX_TEMP:
		value = get_temp_units(stats_interval.max_temp, NULL);
		if (value > -100.0)
			ret =  QString::number(value, 'f', 1);
		break;
	}
	return ret;
}

YearlyStatisticsModel::YearlyStatisticsModel(QObject* parent)
{
	columns = COLUMNS;
	update_yearly_stats();
}

QVariant YearlyStatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant val;
	if (role == Qt::FontRole)
		  val = defaultModelFont();

	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch(section) {
		case YEAR:		val = tr("Year \n > Month / Trip"); break;
		case DIVES:		val = tr("#"); break;
		case TOTAL_TIME:	val = tr("Duration \n Total"); break;
		case AVERAGE_TIME:	val = tr("\nAverage"); break;
		case SHORTEST_TIME:	val = tr("\nShortest"); break;
		case LONGEST_TIME:	val = tr("\nLongest"); break;
		case AVG_DEPTH:		val = QString(tr("Depth (%1)\n Average")).arg(get_depth_unit()); break;
		case MIN_DEPTH:		val = tr("\nMinimum"); break;
		case MAX_DEPTH:		val = tr("\nMaximum"); break;
		case AVG_SAC:		val = QString(tr("SAC (%1)\n Average")).arg(get_volume_unit()); break;
		case MIN_SAC:		val = tr("\nMinimum"); break;
		case MAX_SAC:		val = tr("\nMaximum"); break;
		case AVG_TEMP:		val = QString(tr("Temp. (%1)\n Average").arg(get_temp_unit())); break;
		case MIN_TEMP:		val = tr("\nMinimum"); break;
		case MAX_TEMP:		val = tr("\nMaximum"); break;
		}
	}
	return val;
}

void YearlyStatisticsModel::update_yearly_stats()
{
	int i, month = 0;
	unsigned int j, combined_months;

	for (i = 0; stats_yearly != NULL && stats_yearly[i].period; ++i) {
		YearStatisticsItem *item = new YearStatisticsItem(stats_yearly[i]);
		combined_months = 0;
		for (j = 0; combined_months < stats_yearly[i].selection_size; ++j) {
			combined_months += stats_monthly[month].selection_size;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats_monthly[month]);
			item->children.append(iChild);
			iChild->parent = item;
			month++;
		}
		rootItem->children.append(item);
		item->parent = rootItem;
	}


	if (stats_by_trip != NULL ) {
		YearStatisticsItem *item = new YearStatisticsItem(stats_by_trip[0]);
		for (i = 1; stats_by_trip != NULL && stats_by_trip[i].is_trip; ++i) {
			YearStatisticsItem *iChild = new YearStatisticsItem(stats_by_trip[i]);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem;
	}
}

/*#################################################################
 * #
 * #	Table Print Model
 * #
 * ################################################################
 */
TablePrintModel::TablePrintModel()
{
	columns = 7;
	rows = 0;
}

TablePrintModel::~TablePrintModel()
{
	for (int i = 0; i < list.size(); i++)
		delete list.at(i);
}

void TablePrintModel::insertRow(int index)
{
	struct TablePrintItem *item = new struct TablePrintItem();
	item->colorBackground = 0xffffffff;
	if (index == -1) {
		beginInsertRows(QModelIndex(), rows, rows);
		list.append(item);
	} else {
		beginInsertRows(QModelIndex(), index, index);
		list.insert(index, item);
	}
	endInsertRows();
	rows++;
}

void TablePrintModel::callReset()
{
	reset();
}

QVariant TablePrintModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::BackgroundRole)
		return QColor(list.at(index.row())->colorBackground);
	if (role == Qt::DisplayRole)
		switch (index.column()) {
			case 0: return list.at(index.row())->number;
			case 1: return list.at(index.row())->date;
			case 2:	return list.at(index.row())->depth;
			case 3: return list.at(index.row())->duration;
			case 4:	return list.at(index.row())->divemaster;
			case 5:	return list.at(index.row())->buddy;
			case 6:	return list.at(index.row())->location;
		}
	return QVariant();
}

bool TablePrintModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid()) {
		if (role == Qt::DisplayRole) {
			switch (index.column()) {
			case 0: list.at(index.row())->number = value.toString();
			case 1: list.at(index.row())->date = value.toString();
			case 2: list.at(index.row())->depth = value.toString();
			case 3: list.at(index.row())->duration = value.toString();
			case 4: list.at(index.row())->divemaster = value.toString();
			case 5: list.at(index.row())->buddy = value.toString();
			case 6: list.at(index.row())->location = value.toString();
			}
			return true;
		}
		if (role == Qt::BackgroundRole) {
			list.at(index.row())->colorBackground = value.value<unsigned int>();
			return true;
		}
	}
	return false;
}

int TablePrintModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows;
}

int TablePrintModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return columns;
}

/*#################################################################
 * #
 * #	Profile Print Model
 * #
 * ################################################################
 */

ProfilePrintModel::ProfilePrintModel(QObject *parent)
{
}

/* this is just a helper function to truncate C strings near 'maxlen' characters
 * by finding word bounderies and adding '...' at the end of the truncated string.
 * not really optimal for all languages!
 */
QString ProfilePrintModel::truncateString(char *str, const int maxlen) const
{
	if (!str)
		return QString("");
	QString trunc = QString(str);
	const int len = trunc.length();
	for (int i = 0; i < len; i++) {
		char c = trunc.at(i).toAscii();
		if (c == ' ' || c == '\n' || c == '\t') {
			if (i > maxlen) {
				trunc = trunc.left(i) + QString("...");
				break;
			}
		}
	}
	return trunc;
}

void ProfilePrintModel::setDive(struct dive *divePtr)
{
	dive = divePtr;
	// reset();
}

int ProfilePrintModel::rowCount(const QModelIndex &parent) const
{
	return 11;
}

int ProfilePrintModel::columnCount(const QModelIndex &parent) const
{
	return 7;
}

QVariant ProfilePrintModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int col = index.column();

	switch (role) {
	case Qt::DisplayRole: {
		struct DiveItem di;
		di.dive = dive;
		char buf[80];

		const QString unknown = tr("unknown");

		// dive# + date, depth, location, duration
		if (row == 0) {
			if (col == 0)
				return tr("Dive #%1 - %2").arg(dive->number).arg(di.displayDate());
			if (col == 5) {
				QString unit = (get_units()->length == units::METERS) ? "m" : "ft";
				return tr("Max depth: %1 %2").arg(di.displayDepth()).arg(unit);
			}
		}
		if (row == 1) {
			if (col == 0)
				return truncateString(dive->location, 32);
			if (col == 5)
				return QString(tr("Duration: %1 min")).arg(di.displayDuration());
		}
		// cylinder headings
		if (row == 2) {
			if (col == 0)
				return tr("Cylinder");
			if (col == 1)
				return tr("Gasmix");
			if (col == 2)
				return tr("Gas Used");
		}
		// cylinder data
		if (row > 2 && row < 10 && row - 3 < MAX_CYLINDERS) {
			cylinder_t *cyl = &dive->cylinder[row - 3];
			if (cyl->type.description) { // how do we check if a cylinder is added?
				if (col == 0) {
					if (cyl->type.description[0] != '\0')
						return QString(cyl->type.description);
					return unknown;
				}
				if (col == 1) {
					get_gas_string(cyl->gasmix.o2.permille, cyl->gasmix.he.permille, buf, sizeof(buf));
					return QString(buf);
				}
				if (col == 2) {
					return get_cylinder_used_gas_string(cyl, true);
				}
			}
		}
		// dive notes
		if (row == 10 && col == 0)
			return truncateString(dive->notes, 640);
		// sac, cns, otu - headings
		if (col == 3) {
			if (row == 2)
				return tr("SAC");
			if (row == 4)
				return tr("Max. CNS");
			if (row == 6)
				return tr("OTU");
		}
		// sac, cns, otu - data
		if (col == 4) {
			if (row == 2)
				return di.displaySac();
			if (row == 4)
				return QString::number(dive->maxcns);
			if (row == 6)
				return QString::number(dive->otu);
		}
		// weights heading
		if (row == 2 && col == 5)
			return tr("Weights");
		// total weight
		if (row == 9) {
			weight_t tw = { total_weight(dive) };
			if (tw.grams) {
				if (col == 5)
					return tr("Total weight");
				if (col == 6)
					return get_weight_string(tw, true);
			}
		}
		// weight data
		if (row > 2 && row < 10 && row - 3 < MAX_WEIGHTSYSTEMS) {
			weightsystem_t *ws = &dive->weightsystem[row - 3];
			if (ws->weight.grams) {
				if (col == 5) {
					if (ws->description && ws->description[0] != '\0')
						return QString(ws->description);
					return unknown;
				}
				if (col == 6) {
					return get_weight_string(ws->weight, true);
				}
			}
		}
		return QString();
	}
	case Qt::FontRole: {
		QFont font;
		const int baseSize = 9;
		// dive #
		if (row == 0 && col == 0) {
			font.setBold(true);
			font.setPixelSize(baseSize + 1);
			return QVariant::fromValue(font);
		}
		// dive location
		if (row == 1 && col == 0) {
			font.setPixelSize(baseSize);
			font.setBold(true);
			return QVariant::fromValue(font);
		}
		// depth/duration
		if ((row == 0 || row == 1) && col == 5) {
			font.setPixelSize(baseSize);
			return QVariant::fromValue(font);
		}
		// notes
		if (row == 9 && col == 0) {
			font.setPixelSize(baseSize + 1);
			return QVariant::fromValue(font);
		}
		font.setPixelSize(baseSize);
		return QVariant::fromValue(font);
	}
	case Qt::TextAlignmentRole: {
		unsigned int align = Qt::AlignCenter;
		// dive #, location, notes
		if ((row < 2 || row == 10) && col == 0)
			align = Qt::AlignLeft | Qt::AlignVCenter;
		// depth, duration
		if (row < 2 && col == 5)
			align = Qt::AlignRight | Qt::AlignVCenter;
		return QVariant::fromValue(align);
	}
	} // switch (role)
	return QVariant();
}

Qt::ItemFlags GasSelectionModel::flags(const QModelIndex& index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

GasSelectionModel* GasSelectionModel::instance()
{
	static GasSelectionModel* self = new GasSelectionModel();
	return self;
}

void GasSelectionModel::repopulate()
{
	setStringList(DivePlannerPointsModel::instance()->getGasList());
}

QVariant GasSelectionModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::FontRole){
		return defaultModelFont();
	}
	return QStringListModel::data(index, role);
}
