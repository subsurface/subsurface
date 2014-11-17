/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "models.h"
#include "diveplanner.h"
#include "mainwindow.h"
#include "helpers.h"
#include "dive.h"
#include "device.h"
#include "statistics.h"
#include "qthelper.h"
#include "gettextfromc.h"
#include "display.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QColor>
#include <QBrush>
#include <QFont>
#include <QIcon>
#include <QMessageBox>
#include <QStringListModel>

CleanerTableModel::CleanerTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

int CleanerTableModel::columnCount(const QModelIndex &parent) const
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

void CleanerTableModel::setHeaderDataStrings(const QStringList &newHeaders)
{
	headers = newHeaders;
}

static QPixmap *trashIconPixmap;

// initialize the trash icon if necessary
static void initTrashIcon()
{
	if (!trashIconPixmap)
		trashIconPixmap = new QPixmap(QIcon(":trash").pixmap(defaultIconMetrics().sz_small));
}

const QPixmap &trashIcon()
{
	return *trashIconPixmap;
}

CylindersModel::CylindersModel(QObject *parent) : changed(false),
	rows(0)
{
	//	enum {REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, DEPTH};
	setHeaderDataStrings(QStringList() << "" << tr("Type") << tr("Size") << tr("Work press.") << tr("Start press.") << tr("End press.") << trUtf8("O" UTF8_SUBSCRIPT_2 "%") << tr("He%")
					   << tr("Switch at") << tr("Use"));

	initTrashIcon();
}

CylindersModel *CylindersModel::instance()
{

	static QScopedPointer<CylindersModel> self(new CylindersModel());
	return self.data();
}

static QVariant percent_string(fraction_t fraction)
{
	int permille = fraction.permille;

	if (!permille)
		return QVariant();
	return QString("%1%").arg(permille / 10.0, 0, 'f', 1);
}

QVariant CylindersModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;

	if (!index.isValid() || index.row() >= MAX_CYLINDERS)
		return ret;

	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];
	switch (role) {
	case Qt::BackgroundRole: {
		switch (index.column()) {
		// mark the cylinder start / end pressure in red if the values
		// seem implausible
		case START:
		case END:
			if ((cyl->start.mbar && !cyl->end.mbar) ||
			    (cyl->end.mbar && cyl->start.mbar <= cyl->end.mbar))
				ret = REDORANGE1_HIGH_TRANS;
			else
				ret = WHITE1;
			break;
		}
		break;
	}
	case Qt::FontRole: {
		QFont font = defaultModelFont();
		switch (index.column()) {
		case START:
			font.setItalic(!cyl->start.mbar);
			break;
		case END:
			font.setItalic(!cyl->end.mbar);
			break;
		}
		ret = font;
		break;
	}
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			ret = QString(cyl->type.description);
			break;
		case SIZE:
			if (cyl->type.size.mliter)
				ret = get_volume_string(cyl->type.size, true, cyl->type.workingpressure.mbar);
			break;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar)
				ret = get_pressure_string(cyl->type.workingpressure, true);
			break;
		case START:
			if (cyl->start.mbar)
				ret = get_pressure_string(cyl->start, true);
			else if (cyl->sample_start.mbar)
				ret = get_pressure_string(cyl->sample_start, true);
			break;
		case END:
			if (cyl->end.mbar)
				ret = get_pressure_string(cyl->end, true);
			else if (cyl->sample_end.mbar)
				ret = get_pressure_string(cyl->sample_end, true);
			break;
		case O2:
			ret = percent_string(cyl->gasmix.o2);
			break;
		case HE:
			ret = percent_string(cyl->gasmix.he);
			break;
		case DEPTH:
			ret = get_depth_string(cyl->depth, true);
			break;
		case USE:
			ret = gettextFromC::instance()->trGettext(cylinderuse_text[cyl->cylinder_use]);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			ret = trashIcon().size();
		break;

	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this cylinder.");
		break;
	}

	return ret;
}

cylinder_t *CylindersModel::cylinderAt(const QModelIndex &index)
{
	return &displayed_dive.cylinder[index.row()];
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void CylindersModel::passInData(const QModelIndex &index, const QVariant &value)
{
	cylinder_t *cyl = cylinderAt(index);
	switch (index.column()) {
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

/* Has the string value changed */
#define CHANGED() \
	(vString = value.toString()) != data(index, role).toString()

bool CylindersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString;
	bool addDiveMode = DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING;
	if (addDiveMode)
		DivePlannerPointsModel::instance()->rememberTanks();

	cylinder_t *cyl = cylinderAt(index);
	switch (index.column()) {
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
		if (CHANGED()) {
			TankInfoModel *tanks = TankInfoModel::instance();
			QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, cyl->type.description);

			cyl->type.size = string_to_volume(vString.toUtf8().data(), cyl->type.workingpressure);
			mark_divelist_changed(true);
			if (!matches.isEmpty())
				tanks->setData(tanks->index(matches.first().row(), TankInfoModel::ML), cyl->type.size.mliter);
			changed = true;
		}
		break;
	case WORKINGPRESS:
		if (CHANGED()) {
			TankInfoModel *tanks = TankInfoModel::instance();
			QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, cyl->type.description);
			cyl->type.workingpressure = string_to_pressure(vString.toUtf8().data());
			if (!matches.isEmpty())
				tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
			changed = true;
		}
		break;
	case START:
		if (CHANGED()) {
			cyl->start = string_to_pressure(vString.toUtf8().data());
			changed = true;
		}
		break;
	case END:
		if (CHANGED()) {
			//&& (!cyl->start.mbar || string_to_pressure(vString.toUtf8().data()).mbar <= cyl->start.mbar)) {
			cyl->end = string_to_pressure(vString.toUtf8().data());
			changed = true;
		}
		break;
	case O2:
		if (CHANGED()) {
			cyl->gasmix.o2 = string_to_fraction(vString.toUtf8().data());
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&cyl->gasmix, modpO2, M_OR_FT(3, 10));
			changed = true;
		}
		break;
	case HE:
		if (CHANGED()) {
			cyl->gasmix.he = string_to_fraction(vString.toUtf8().data());
			changed = true;
		}
		break;
	case DEPTH:
		if (CHANGED()) {
			cyl->depth = string_to_depth(vString.toUtf8().data());
			changed = true;
		}
		break;
	case USE:
		if (CHANGED()) {
			cyl->cylinder_use = (enum cylinderuse)vString.toInt();
			changed = true;
		}
		break;
	}
	if (addDiveMode)
		DivePlannerPointsModel::instance()->tanksUpdated();
	dataChanged(index, index);
	return true;
}

int CylindersModel::rowCount(const QModelIndex &parent) const
{
	return rows;
}

void CylindersModel::add()
{
	if (rows >= MAX_CYLINDERS) {
		return;
	}

	int row = rows;
	fill_default_cylinder(&displayed_dive.cylinder[row]);
	displayed_dive.cylinder[row].manually_added = true;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
	endInsertRows();
}

void CylindersModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

void CylindersModel::updateDive()
{
	clear();
	rows = 0;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!cylinder_none(&displayed_dive.cylinder[i]) &&
		    (prefs.display_unused_tanks ||
		     is_cylinder_used(&displayed_dive, i) ||
		     displayed_dive.cylinder[i].manually_added))
			rows = i + 1;
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

void CylindersModel::copyFromDive(dive *d)
{
	if (!d)
		return;
	rows = 0;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!cylinder_none(&d->cylinder[i]) && is_cylinder_used(d, i)) {
			rows = i + 1;
		}
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void CylindersModel::remove(const QModelIndex &index)
{
	if (index.column() != REMOVE) {
		return;
	}
	int same_gas = -1;
	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];
	struct gasmix *mygas = &cyl->gasmix;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (i == index.row() || cylinder_none(&displayed_dive.cylinder[i]))
			continue;
		struct gasmix *gas2 = &displayed_dive.cylinder[i].gasmix;
		if (gasmix_distance(mygas, gas2) == 0)
			same_gas = i;
	}
	if (same_gas == -1 &&
	    ((DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING &&
	      DivePlannerPointsModel::instance()->tankInUse(cyl->gasmix)) ||
	     (DivePlannerPointsModel::instance()->currentMode() == DivePlannerPointsModel::NOTHING &&
	      (cyl->manually_added || is_cylinder_used(&displayed_dive, index.row()))))) {
		QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(
								     tr("Cylinder cannot be removed"),
								     tr("This gas is in use. Only cylinders that are not used in the dive can be removed.")),
				     QMessageBox::Ok);
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	if (index.row() == 0) {
		// first gas - we need to make sure that the same gas ends up
		// as first gas
		memmove(cyl, &displayed_dive.cylinder[same_gas], sizeof(*cyl));
		remove_cylinder(&displayed_dive, same_gas);
	} else {
		remove_cylinder(&displayed_dive, index.row());
	}
	changed = true;
	endRemoveRows();
}

WeightModel::WeightModel(QObject *parent) : CleanerTableModel(parent),
	changed(false),
	rows(0)
{
	//enum Column {REMOVE, TYPE, WEIGHT};
	setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Weight"));

	initTrashIcon();
}

weightsystem_t *WeightModel::weightSystemAt(const QModelIndex &index)
{
	return &displayed_dive.weightsystem[index.row()];
}

void WeightModel::remove(const QModelIndex &index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_weightsystem(&displayed_dive, index.row());
	changed = true;
	endRemoveRows();
}

void WeightModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

QVariant WeightModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_WEIGHTSYSTEMS)
		return ret;

	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			ret = gettextFromC::instance()->tr(ws->description);
			break;
		case WEIGHT:
			ret = get_weight_string(ws->weight, true);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			ret = trashIcon().size();
		break;
	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this weight system.");
		break;
	}
	return ret;
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void WeightModel::passInData(const QModelIndex &index, const QVariant &value)
{
	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];
	if (index.column() == WEIGHT) {
		if (ws->weight.grams != value.toInt()) {
			ws->weight.grams = value.toInt();
			dataChanged(index, index);
		}
	}
}
//TODO: Move to C
weight_t string_to_weight(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	QString rest = QString(end).trimmed();
	QString local_kg = QObject::tr("kg");
	QString local_lbs = QObject::tr("lbs");
	weight_t weight;

	if (rest.startsWith("kg") || rest.startsWith(local_kg))
		goto kg;
	// using just "lb" instead of "lbs" is intentional - some people might enter the singular
	if (rest.startsWith("lb") || rest.startsWith(local_lbs))
		goto lbs;
	if (prefs.units.weight == prefs.units.LBS)
		goto lbs;
kg:
	weight.grams = rint(value * 1000);
	return weight;
lbs:
	weight.grams = lbs_to_grams(value);
	return weight;
}

//TODO: Move to C.
depth_t string_to_depth(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	QString rest = QString(end).trimmed();
	QString local_ft = QObject::tr("ft");
	QString local_m = QObject::tr("m");
	depth_t depth;

	if (rest.startsWith("m") || rest.startsWith(local_m))
		goto m;
	if (rest.startsWith("ft") || rest.startsWith(local_ft))
		goto ft;
	if (prefs.units.length == prefs.units.FEET)
		goto ft;
m:
	depth.mm = rint(value * 1000);
	return depth;
ft:
	depth.mm = feet_to_mm(value);
	return depth;
}

//TODO: Move to C.
pressure_t string_to_pressure(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	QString rest = QString(end).trimmed();
	QString local_psi = QObject::tr("psi");
	QString local_bar = QObject::tr("bar");
	pressure_t pressure;

	if (rest.startsWith("bar") || rest.startsWith(local_bar))
		goto bar;
	if (rest.startsWith("psi") || rest.startsWith(local_psi))
		goto psi;
	if (prefs.units.pressure == prefs.units.PSI)
		goto psi;
bar:
	pressure.mbar = rint(value * 1000);
	return pressure;
psi:
	pressure.mbar = psi_to_mbar(value);
	return pressure;
}

//TODO: Move to C.
/* Imperial cylinder volumes need working pressure to be meaningful */
volume_t string_to_volume(const char *str, pressure_t workp)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	QString rest = QString(end).trimmed();
	QString local_l = QObject::tr("l");
	QString local_cuft = QObject::tr("cuft");
	volume_t volume;

	if (rest.startsWith("l") || rest.startsWith("ℓ") || rest.startsWith(local_l))
		goto l;
	if (rest.startsWith("cuft") || rest.startsWith(local_cuft))
		goto cuft;
	/*
	 * If we don't have explicit units, and there is no working
	 * pressure, we're going to assume "liter" even in imperial
	 * measurements.
	 */
	if (!workp.mbar)
		goto l;
	if (prefs.units.volume == prefs.units.LITER)
		goto l;
cuft:
	if (workp.mbar)
		value /= bar_to_atm(workp.mbar / 1000.0);
	value = cuft_to_l(value);
l:
	volume.mliter = rint(value * 1000);
	return volume;
}

//TODO: Move to C.
fraction_t string_to_fraction(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	fraction_t fraction;

	fraction.permille = rint(value * 10);
	return fraction;
}

bool WeightModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString = value.toString();
	weightsystem_t *ws = &displayed_dive.weightsystem[index.row()];
	switch (index.column()) {
	case TYPE:
		if (!value.isNull()) {
			//TODO: C-function weigth_system_set_description ?
			if (!ws->description || gettextFromC::instance()->tr(ws->description) != vString) {
				// loop over translations to see if one matches
				int i = -1;
				while (ws_info[++i].name) {
					if (gettextFromC::instance()->tr(ws_info[i].name) == vString) {
						ws->description = copy_string(ws_info[i].name);
						break;
					}
				}
				if (ws_info[i].name == NULL) // didn't find a match
					ws->description = strdup(vString.toUtf8().constData());
				changed = true;
			}
		}
		break;
	case WEIGHT:
		if (CHANGED()) {
			ws->weight = string_to_weight(vString.toUtf8().data());
			// now update the ws_info
			changed = true;
			WSInfoModel *wsim = WSInfoModel::instance();
			QModelIndexList matches = wsim->match(wsim->index(0, 0), Qt::DisplayRole, gettextFromC::instance()->tr(ws->description));
			if (!matches.isEmpty())
				wsim->setData(wsim->index(matches.first().row(), WSInfoModel::GR), ws->weight.grams);
		}
		break;
	}
	dataChanged(index, index);
	return true;
}

Qt::ItemFlags WeightModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int WeightModel::rowCount(const QModelIndex &parent) const
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

void WeightModel::updateDive()
{
	clear();
	rows = 0;
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		if (!weightsystem_none(&displayed_dive.weightsystem[i])) {
			rows = i + 1;
		}
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

WSInfoModel *WSInfoModel::instance()
{
	static QScopedPointer<WSInfoModel> self(new WSInfoModel());
	return self.data();
}

bool WSInfoModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool WSInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	struct ws_info_t *info = &ws_info[index.row()];
	switch (index.column()) {
	case DESCRIPTION:
		info->name = strdup(value.toByteArray().data());
		break;
	case GR:
		info->grams = value.toInt();
		break;
	}
	emit dataChanged(index, index);
	return true;
}

void WSInfoModel::clear()
{
}

QVariant WSInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	struct ws_info_t *info = &ws_info[index.row()];

	int gr = info->grams;
	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case GR:
			ret = gr;
			break;
		case DESCRIPTION:
			ret = gettextFromC::instance()->tr(info->name);
			break;
		}
		break;
	}
	return ret;
}

int WSInfoModel::rowCount(const QModelIndex &parent) const
{
	return rows + 1;
}

const QString &WSInfoModel::biggerString() const
{
	return biggerEntry;
}

WSInfoModel::WSInfoModel() : rows(-1)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("kg"));
	struct ws_info_t *info = ws_info;
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
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
	for (info = ws_info; info->name; info++, rows++) {
		QString wsInfoName = gettextFromC::instance()->tr(info->name);
		if (wsInfoName.count() > biggerEntry.count())
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
	for (info = ws_info; info->name; info++, rows++)
		;

	if (rows > -1) {
		beginInsertRows(QModelIndex(), 0, rows);
		endInsertRows();
	}
}

TankInfoModel *TankInfoModel::instance()
{
	static QScopedPointer<TankInfoModel> self(new TankInfoModel());
	return self.data();
}

const QString &TankInfoModel::biggerString() const
{
	return biggerEntry;
}

bool TankInfoModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, rowCount(), rowCount());
	rows += count;
	endInsertRows();
	return true;
}

bool TankInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	struct tank_info_t *info = &tank_info[index.row()];
	switch (index.column()) {
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
	return true;
}

void TankInfoModel::clear()
{
}

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid()) {
		return ret;
	}
	if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		struct tank_info_t *info = &tank_info[index.row()];
		int ml = info->ml;
		double bar = (info->psi) ? psi_to_bar(info->psi) : info->bar;

		if (info->cuft && info->psi)
			ml = cuft_to_l(info->cuft) * 1000 / bar_to_atm(bar);

		switch (index.column()) {
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

int TankInfoModel::rowCount(const QModelIndex &parent) const
{
	return rows + 1;
}

TankInfoModel::TankInfoModel() : rows(-1)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("ml") << tr("bar"));
	struct tank_info_t *info = tank_info;
	for (info = tank_info; info->name; info++, rows++) {
		QString infoName = gettextFromC::instance()->tr(info->name);
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
	for (info = tank_info; info->name; info++, rows++)
		;

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

Qt::ItemFlags TreeItem::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int TreeItem::row() const
{
	if (parent)
		return parent->children.indexOf(const_cast<TreeItem *>(this));
	return 0;
}

QVariant TreeItem::data(int column, int role) const
{
	return QVariant();
}

TreeModel::TreeModel(QObject *parent) : QAbstractItemModel(parent)
{
	columns = 0; // I'm not sure about this one - I can't see where it gets initialized
	rootItem = new TreeItem();
}

TreeModel::~TreeModel()
{
	delete rootItem;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
	QVariant val = item->data(index.column(), role);

	if (role == Qt::FontRole && !val.isValid())
		return defaultModelFont();
	else
		return val;
}

bool TreeItem::setData(const QModelIndex &index, const QVariant &value, int role)
{
	return false;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem *parentItem = (!parent.isValid()) ? rootItem : static_cast<TreeItem *>(parent.internalPointer());

	TreeItem *childItem = parentItem->children[row];

	return (childItem) ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem *childItem = static_cast<TreeItem *>(index.internalPointer());
	TreeItem *parentItem = childItem->parent;

	if (parentItem == rootItem || !parentItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
	TreeItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<TreeItem *>(parent.internalPointer());

	int amount = parentItem->children.count();
	return amount;
}

int TreeModel::columnCount(const QModelIndex &parent) const
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
	dive_trip_t *trip;
};

QVariant TripItem::data(int column, int role) const
{
	QVariant ret;

	if (role == DiveTripModel::TRIP_ROLE)
		return QVariant::fromValue<void *>(trip);

	if (role == DiveTripModel::SORT_ROLE)
		return (qulonglong)trip->when;

	if (role == Qt::DisplayRole) {
		switch (column) {
		case DiveTripModel::NR:
			QString shownText;
			struct dive *d = trip->dives;
			int countShown = 0;
			while (d) {
				if (!d->hidden_by_filter)
					countShown++;
				d = d->next;
			}
			if (countShown < trip->nrdives)
				shownText = tr(" (%1 shown)").arg(countShown);
			if (trip->location && *trip->location)
				ret = QString(trip->location) + ", " + get_trip_date_string(trip->when, trip->nrdives) + shownText;
			else
				ret = get_trip_date_string(trip->when, trip->nrdives) + shownText;
			break;
		}
	}

	return ret;
}

static int nitrox_sort_value(struct dive *dive)
{
	int o2, he, o2low;
	get_dive_gas(dive, &o2, &he, &o2low);
	return he * 1000 + o2;
}

static QVariant dive_table_alignment(int column)
{
	QVariant retVal;
	switch (column) {
	case DiveTripModel::DEPTH:
	case DiveTripModel::DURATION:
	case DiveTripModel::TEMPERATURE:
	case DiveTripModel::TOTALWEIGHT:
	case DiveTripModel::SAC:
	case DiveTripModel::OTU:
	case DiveTripModel::MAXCNS:
		// Right align numeric columns
		retVal = int(Qt::AlignRight | Qt::AlignVCenter);
		break;
	// NR needs to be left aligned becase its the indent marker for trips too
	case DiveTripModel::NR:
	case DiveTripModel::DATE:
	case DiveTripModel::RATING:
	case DiveTripModel::SUIT:
	case DiveTripModel::CYLINDER:
	case DiveTripModel::GAS:
	case DiveTripModel::LOCATION:
		retVal = int(Qt::AlignLeft | Qt::AlignVCenter);
		break;
	}
	return retVal;
}

QVariant DiveItem::data(int column, int role) const
{
	QVariant retVal;
	struct dive *dive = get_dive_by_uniq_id(diveId);

	switch (role) {
	case Qt::TextAlignmentRole:
		retVal = dive_table_alignment(column);
		break;
	case DiveTripModel::SORT_ROLE:
		Q_ASSERT(dive != NULL);
		switch (column) {
		case NR:
			retVal = (qulonglong)dive->when;
			break;
		case DATE:
			retVal = (qulonglong)dive->when;
			break;
		case RATING:
			retVal = dive->rating;
			break;
		case DEPTH:
			retVal = dive->maxdepth.mm;
			break;
		case DURATION:
			retVal = dive->duration.seconds;
			break;
		case TEMPERATURE:
			retVal = dive->watertemp.mkelvin;
			break;
		case TOTALWEIGHT:
			retVal = total_weight(dive);
			break;
		case SUIT:
			retVal = QString(dive->suit);
			break;
		case CYLINDER:
			retVal = QString(dive->cylinder[0].type.description);
			break;
		case GAS:
			retVal = nitrox_sort_value(dive);
			break;
		case SAC:
			retVal = dive->sac;
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
	case Qt::DisplayRole:
		Q_ASSERT(dive != NULL);
		switch (column) {
		case NR:
			retVal = dive->number;
			break;
		case DATE:
			retVal = displayDate();
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
		case GAS:
			retVal = QString(get_dive_gas_string(dive));
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
	case Qt::ToolTipRole:
		switch (column) {
		case NR:
			retVal = tr("#");
			break;
		case DATE:
			retVal = tr("Date");
			break;
		case RATING:
			retVal = tr("Rating");
			break;
		case DEPTH:
			retVal = tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
			break;
		case DURATION:
			retVal = tr("Duration");
			break;
		case TEMPERATURE:
			retVal = tr("Temp(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			retVal = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			retVal = tr("Suit");
			break;
		case CYLINDER:
			retVal = tr("Cyl");
			break;
		case GAS:
			retVal = tr("Gas");
			break;
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			retVal = tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
			break;
		case OTU:
			retVal = tr("OTU");
			break;
		case MAXCNS:
			retVal = tr("Max CNS");
			break;
		case LOCATION:
			retVal = tr("Location");
			break;
		}
		break;
	}

	if (role == DiveTripModel::STAR_ROLE) {
		Q_ASSERT(dive != NULL);
		retVal = dive->rating;
	}
	if (role == DiveTripModel::DIVE_ROLE) {
		retVal = QVariant::fromValue<void *>(dive);
	}
	if (role == DiveTripModel::DIVE_IDX) {
		Q_ASSERT(dive != NULL);
		retVal = get_divenr(dive);
	}
	return retVal;
}

Qt::ItemFlags DiveItem::flags(const QModelIndex &index) const
{
	if (index.column() == NR) {
		return TreeItem::flags(index) | Qt::ItemIsEditable;
	}
	return TreeItem::flags(index);
}

bool DiveItem::setData(const QModelIndex &index, const QVariant &value, int role)
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
	for_each_dive (i, d) {
		if (d->number == v)
			return false;
	}
	d = get_dive_by_uniq_id(diveId);
	d->number = value.toInt();
	mark_divelist_changed(true);
	return true;
}

QString DiveItem::displayDate() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_dive_date_string(dive->when);
}

QString DiveItem::displayDepth() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_depth_string(dive->maxdepth);
}

QString DiveItem::displayDepthWithUnit() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_depth_string(dive->maxdepth, true);
}

QString DiveItem::displayDuration() const
{
	int hrs, mins;
	struct dive *dive = get_dive_by_uniq_id(diveId);
	mins = (dive->duration.seconds + 59) / 60;
	hrs = mins / 60;
	mins -= hrs * 60;

	QString displayTime;
	if (hrs)
		displayTime = QString("%1:%2").arg(hrs).arg(mins, 2, 10, QChar('0'));
	else
		displayTime = QString("%1").arg(mins);
	return displayTime;
}

QString DiveItem::displayTemperature() const
{
	QString str;
	struct dive *dive = get_dive_by_uniq_id(diveId);
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
	struct dive *dive = get_dive_by_uniq_id(diveId);
	if (dive->sac) {
		const char *unit;
		int decimal;
		double value = get_volume_units(dive->sac, &decimal, &unit);
		return QString::number(value, 'f', decimal);
	}
	return QString("");
}

QString DiveItem::displayWeight() const
{
	QString str = weight_string(weight());
	return str;
}

int DiveItem::weight() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	weight_t tw = { total_weight(dive) };
	return tw.grams;
}

DiveTripModel::DiveTripModel(QObject *parent) : TreeModel(parent)
{
	columns = COLUMNS;
}

Qt::ItemFlags DiveTripModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	TripItem *item = static_cast<TripItem *>(index.internalPointer());
	return item->flags(index);
}

QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::TextAlignmentRole:
		ret = dive_table_alignment(section);
		break;
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
		switch (section) {
		case NR:
			ret = tr("#");
			break;
		case DATE:
			ret = tr("Date");
			break;
		case RATING:
			ret = tr("Rating");
			break;
		case DEPTH:
			ret = tr("Depth");
			break;
		case DURATION:
			ret = tr("Duration");
			break;
		case TEMPERATURE:
			ret = tr("Temp");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight");
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cyl");
			break;
		case GAS:
			ret = tr("Gas");
			break;
		case SAC:
			ret = tr("SAC");
			break;
		case OTU:
			ret = tr("OTU");
			break;
		case MAXCNS:
			ret = tr("Max CNS");
			break;
		case LOCATION:
			ret = tr("Location");
			break;
		}
		break;
	case Qt::ToolTipRole:
		switch (section) {
		case NR:
			ret = tr("#");
			break;
		case DATE:
			ret = tr("Date");
			break;
		case RATING:
			ret = tr("Rating");
			break;
		case DEPTH:
			ret = tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
			break;
		case DURATION:
			ret = tr("Duration");
			break;
		case TEMPERATURE:
			ret = tr("Temp(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cyl");
			break;
		case GAS:
			ret = tr("Gas");
			break;
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			ret = tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
			break;
		case OTU:
			ret = tr("OTU");
			break;
		case MAXCNS:
			ret = tr("Max CNS");
			break;
		case LOCATION:
			ret = tr("Location");
			break;
		}
		break;
	}

	return ret;
}

void DiveTripModel::setupModelData()
{
	int i = dive_table.nr;

	if (rowCount()) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		endRemoveRows();
	}

	if (autogroup)
		autogroup_dives();
	dive_table.preexisting = dive_table.nr;
	while (--i >= 0) {
		struct dive *dive = get_dive(i);
		update_cylinder_related_info(dive);
		dive_trip_t *trip = dive->divetrip;

		DiveItem *diveItem = new DiveItem();
		diveItem->diveId = dive->id;

		if (!trip || currentLayout == LIST) {
			diveItem->parent = rootItem;
			rootItem->children.push_back(diveItem);
			continue;
		}
		if (currentLayout == LIST)
			continue;

		if (!trips.keys().contains(trip)) {
			TripItem *tripItem = new TripItem();
			tripItem->trip = trip;
			tripItem->parent = rootItem;
			tripItem->children.push_back(diveItem);
			trips[trip] = tripItem;
			rootItem->children.push_back(tripItem);
			continue;
		}
		TripItem *tripItem = trips[trip];
		tripItem->children.push_back(diveItem);
	}

	if (rowCount()) {
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

bool DiveTripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
	DiveItem *diveItem = dynamic_cast<DiveItem *>(item);
	if (!diveItem)
		return false;
	return diveItem->setData(index, value, role);
}


/*####################################################################
 *
 * Dive Computer Model
 *
 *####################################################################
 */

DiveComputerModel::DiveComputerModel(QMultiMap<QString, DiveComputerNode> &dcMap, QObject *parent) : CleanerTableModel()
{
	setHeaderDataStrings(QStringList() << "" << tr("Model") << tr("Device ID") << tr("Nickname"));
	dcWorkingMap = dcMap;
	numRows = 0;

	initTrashIcon();
}

QVariant DiveComputerModel::data(const QModelIndex &index, int role) const
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());

	QVariant ret;
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case ID:
			ret = QString("0x").append(QString::number(node.deviceId, 16));
			break;
		case MODEL:
			ret = node.model;
			break;
		case NICKNAME:
			ret = node.nickName;
			break;
		}
	}

	if (index.column() == REMOVE) {
		switch (role) {
		case Qt::DecorationRole:
			ret = trashIcon();
			break;
		case Qt::SizeHintRole:
			ret = trashIcon().size();
			break;
		case Qt::ToolTipRole:
			ret = tr("Clicking here will remove this dive computer.");
			break;
		}
	}
	return ret;
}

int DiveComputerModel::rowCount(const QModelIndex &parent) const
{
	return numRows;
}

void DiveComputerModel::update()
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	int count = values.count();

	if (numRows) {
		beginRemoveRows(QModelIndex(), 0, numRows - 1);
		numRows = 0;
		endRemoveRows();
	}

	if (count) {
		beginInsertRows(QModelIndex(), 0, count - 1);
		numRows = count;
		endInsertRows();
	}
}

Qt::ItemFlags DiveComputerModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.column() == NICKNAME)
		flags |= Qt::ItemIsEditable;
	return flags;
}

bool DiveComputerModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QList<DiveComputerNode> values = dcWorkingMap.values();
	DiveComputerNode node = values.at(index.row());
	dcWorkingMap.remove(node.model, node);
	node.nickName = value.toString();
	dcWorkingMap.insert(node.model, node);
	emit dataChanged(index, index);
	return true;
}

void DiveComputerModel::remove(const QModelIndex &index)
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
		mark_divelist_changed(true);
	dcList.dcMap = dcWorkingMap;
}

/*#################################################################
 * #
 * #	Yearly Statistics Model
 * #
 * ################################################################
 */

class YearStatisticsItem : public TreeItem {
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
	switch (column) {
	case YEAR:
		if (stats_interval.is_trip) {
			ret = stats_interval.location;
		} else {
			ret = stats_interval.period;
		}
		break;
	case DIVES:
		ret = stats_interval.selection_size;
		break;
	case TOTAL_TIME:
		ret = get_time_string(stats_interval.total_time.seconds, 0);
		break;
	case AVERAGE_TIME:
		ret = get_minutes(stats_interval.total_time.seconds / stats_interval.selection_size);
		break;
	case SHORTEST_TIME:
		ret = get_minutes(stats_interval.shortest_time.seconds);
		break;
	case LONGEST_TIME:
		ret = get_minutes(stats_interval.longest_time.seconds);
		break;
	case AVG_DEPTH:
		ret = get_depth_string(stats_interval.avg_depth);
		break;
	case MIN_DEPTH:
		ret = get_depth_string(stats_interval.min_depth);
		break;
	case MAX_DEPTH:
		ret = get_depth_string(stats_interval.max_depth);
		break;
	case AVG_SAC:
		ret = get_volume_string(stats_interval.avg_sac);
		break;
	case MIN_SAC:
		ret = get_volume_string(stats_interval.min_sac);
		break;
	case MAX_SAC:
		ret = get_volume_string(stats_interval.max_sac);
		break;
	case AVG_TEMP:
		if (stats_interval.combined_temp && stats_interval.combined_count) {
			ret = QString::number(stats_interval.combined_temp / stats_interval.combined_count, 'f', 1);
		}
		break;
	case MIN_TEMP:
		value = get_temp_units(stats_interval.min_temp, NULL);
		if (value > -100.0)
			ret = QString::number(value, 'f', 1);
		break;
	case MAX_TEMP:
		value = get_temp_units(stats_interval.max_temp, NULL);
		if (value > -100.0)
			ret = QString::number(value, 'f', 1);
		break;
	}
	return ret;
}

YearlyStatisticsModel::YearlyStatisticsModel(QObject *parent)
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
		switch (section) {
		case YEAR:
			val = tr("Year \n > Month / Trip");
			break;
		case DIVES:
			val = tr("#");
			break;
		case TOTAL_TIME:
			val = tr("Duration \n Total");
			break;
		case AVERAGE_TIME:
			val = tr("\nAverage");
			break;
		case SHORTEST_TIME:
			val = tr("\nShortest");
			break;
		case LONGEST_TIME:
			val = tr("\nLongest");
			break;
		case AVG_DEPTH:
			val = QString(tr("Depth (%1)\n Average")).arg(get_depth_unit());
			break;
		case MIN_DEPTH:
			val = tr("\nMinimum");
			break;
		case MAX_DEPTH:
			val = tr("\nMaximum");
			break;
		case AVG_SAC:
			val = QString(tr("SAC (%1)\n Average")).arg(get_volume_unit());
			break;
		case MIN_SAC:
			val = tr("\nMinimum");
			break;
		case MAX_SAC:
			val = tr("\nMaximum");
			break;
		case AVG_TEMP:
			val = QString(tr("Temp. (%1)\n Average").arg(get_temp_unit()));
			break;
		case MIN_TEMP:
			val = tr("\nMinimum");
			break;
		case MAX_TEMP:
			val = tr("\nMaximum");
			break;
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


	if (stats_by_trip != NULL && stats_by_trip[0].is_trip == true) {
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
	beginResetModel();
	endResetModel();
}

QVariant TablePrintModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::BackgroundRole)
		return QColor(list.at(index.row())->colorBackground);
	if (role == Qt::DisplayRole)
		switch (index.column()) {
		case 0:
			return list.at(index.row())->number;
		case 1:
			return list.at(index.row())->date;
		case 2:
			return list.at(index.row())->depth;
		case 3:
			return list.at(index.row())->duration;
		case 4:
			return list.at(index.row())->divemaster;
		case 5:
			return list.at(index.row())->buddy;
		case 6:
			return list.at(index.row())->location;
		}
	if (role == Qt::FontRole) {
		QFont font;
		font.setPointSizeF(7.5);
		if (index.row() == 0 && index.column() == 0) {
			font.setBold(true);
		}
		return QVariant::fromValue(font);
	}
	return QVariant();
}

bool TablePrintModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid()) {
		if (role == Qt::DisplayRole) {
			switch (index.column()) {
			case 0:
				list.at(index.row())->number = value.toString();
			case 1:
				list.at(index.row())->date = value.toString();
			case 2:
				list.at(index.row())->depth = value.toString();
			case 3:
				list.at(index.row())->duration = value.toString();
			case 4:
				list.at(index.row())->divemaster = value.toString();
			case 5:
				list.at(index.row())->buddy = value.toString();
			case 6: {
				/* truncate if there are more than N lines of text,
				 * we don't want a row to be larger that a single page! */
				QString s = value.toString();
				const int maxLines = 15;
				int count = 0;
				for (int i = 0; i < s.length(); i++) {
					if (s.at(i) != QChar('\n'))
						continue;
					count++;
					if (count > maxLines) {
						s = s.left(i - 1);
						break;
					}
				}
				list.at(index.row())->location = s;
			}
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

void ProfilePrintModel::setDive(struct dive *divePtr)
{
	diveId = divePtr->id;
	// reset();
}

void ProfilePrintModel::setFontsize(double size)
{
	fontSize = size;
}

int ProfilePrintModel::rowCount(const QModelIndex &parent) const
{
	return 12;
}

int ProfilePrintModel::columnCount(const QModelIndex &parent) const
{
	return 5;
}

QVariant ProfilePrintModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int col = index.column();

	switch (role) {
	case Qt::DisplayRole: {
		struct dive *dive = get_dive_by_uniq_id(diveId);
		struct DiveItem di;
		di.diveId = diveId;

		const QString unknown = tr("unknown");

		// dive# + date, depth, location, duration
		if (row == 0) {
			if (col == 0)
				return tr("Dive #%1 - %2").arg(dive->number).arg(di.displayDate());
			if (col == 3) {
				QString unit = (get_units()->length == units::METERS) ? "m" : "ft";
				return tr("Max depth: %1 %2").arg(di.displayDepth()).arg(unit);
			}
		}
		if (row == 1) {
			if (col == 0)
				return QString(dive->location);
			if (col == 3)
				return QString(tr("Duration: %1 min")).arg(di.displayDuration());
		}
		// headings
		if (row == 2) {
			if (col == 0)
				return tr("Gas used:");
			if (col == 2)
				return tr("Tags:");
			if (col == 3)
				return tr("SAC:");
			if (col == 4)
				return tr("Weights:");
		}
		// notes
		if (col == 0) {
			if (row == 6)
				return tr("Notes:");
			if (row == 7)
				return QString(dive->notes);
		}
		// more headings
		if (row == 4) {
			if (col == 0)
				return tr("Divemaster:");
			if (col == 1)
				return tr("Buddy:");
			if (col == 2)
				return tr("Suit:");
			if (col == 3)
				return tr("Viz:");
			if (col == 4)
				return tr("Rating:");
		}
		// values for gas, sac, etc...
		if (row == 3) {
			if (col == 0) {
				int added = 0;
				QString gas, gases;
				for (int i = 0; i < MAX_CYLINDERS; i++) {
					if (!is_cylinder_used(dive, i))
						continue;
					gas = dive->cylinder[i].type.description;
					gas += QString(!gas.isEmpty() ? " " : "") + gasname(&dive->cylinder[i].gasmix);
					// if has a description and if such gas is not already present
					if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
						if (added > 0)
							gases += QString(" / ");
						gases += gas;
						added++;
					}
				}
				return gases;
			}
			if (col == 2) {
				char buffer[256];
				taglist_get_tagstring(dive->tag_list, buffer, 256);
				return QString(buffer);
			}
			if (col == 3)
				return di.displaySac();
			if (col == 4) {
				weight_t tw = { total_weight(dive) };
				return get_weight_string(tw, true);
			}
		}
		// values for DM, buddy, suit, etc...
		if (row == 5) {
			if (col == 0)
				return QString(dive->divemaster);
			if (col == 1)
				return QString(dive->buddy);
			if (col == 2)
				return QString(dive->suit);
			if (col == 3)
				return (dive->visibility) ? QString::number(dive->visibility).append(" / 5") : QString();
			if (col == 4)
				return (dive->rating) ? QString::number(dive->rating).append(" / 5") : QString();
		}
		return QString();
	}
	case Qt::FontRole: {
		QFont font;
		font.setPointSizeF(fontSize);
		if (row == 0 && col == 0) {
			font.setBold(true);
		}
		return QVariant::fromValue(font);
	}
	case Qt::TextAlignmentRole: {
		// everything is aligned to the left
		unsigned int align = Qt::AlignLeft;
		// align depth and duration right
		if (row < 2 && col == 4)
			align = Qt::AlignRight | Qt::AlignVCenter;
		return QVariant::fromValue(align);
	}
	} // switch (role)
	return QVariant();
}

Qt::ItemFlags GasSelectionModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

GasSelectionModel *GasSelectionModel::instance()
{
	static QScopedPointer<GasSelectionModel> self(new GasSelectionModel());
	return self.data();
}

void GasSelectionModel::repopulate()
{
	setStringList(DivePlannerPointsModel::instance()->getGasList());
}

QVariant GasSelectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QStringListModel::data(index, role);
}

// Language Model, The Model to populate the list of possible Languages.

LanguageModel *LanguageModel::instance()
{
	static LanguageModel *self = new LanguageModel();
	QLocale l;
	return self;
}

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
	QSettings s;
	QDir d(getSubsurfaceDataPath("translations"));
	Q_FOREACH (const QString &s, d.entryList()) {
		if (s.startsWith("subsurface_") && s.endsWith(".qm")) {
			languages.push_back((s == "subsurface_source.qm") ? "English" : s);
		}
	}
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
	QLocale loc;
	QString currentString = languages.at(index.row());
	if (!index.isValid())
		return QVariant();
	switch (role) {
	case Qt::DisplayRole: {
		QLocale l(currentString.remove("subsurface_"));
		return currentString == "English" ? currentString : QString("%1 (%2)").arg(l.languageToString(l.language())).arg(l.countryToString(l.country()));
	}
	case Qt::UserRole:
		return currentString == "English" ? "en_US" : currentString.remove("subsurface_");
	}
	return QVariant();
}

int LanguageModel::rowCount(const QModelIndex &parent) const
{
	return languages.count();
}

ExtraDataModel::ExtraDataModel(QObject *parent) : CleanerTableModel(parent),
	rows(0)
{
	//enum Column {KEY, VALUE};
	setHeaderDataStrings(QStringList() << tr("Key") << tr("Value"));
}

void ExtraDataModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

QVariant ExtraDataModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	int i = -1;
	while (ed && ++i < index.row())
		ed = ed->next;
	if (!ed)
		return ret;

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = int(Qt::AlignLeft | Qt::AlignVCenter);
		break;
	case Qt::DisplayRole:
		switch (index.column()) {
		case KEY:
			ret = QString(ed->key);
			break;
		case VALUE:
			ret = QString(ed->value);
			break;
		}
		break;
	}
	return ret;
}

int ExtraDataModel::rowCount(const QModelIndex &parent) const
{
	return rows;
}

void ExtraDataModel::updateDive()
{
	clear();
	rows = 0;
	struct extra_data *ed = get_dive_dc(&displayed_dive, dc_number)->extra_data;
	while (ed) {
		rows++;
		ed = ed->next;
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}
