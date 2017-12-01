// SPDX-License-Identifier: GPL-2.0
#include "cylindermodel.h"
#include "tankinfomodel.h"
#include "models.h"
#include "core/helpers.h"
#include "core/dive.h"
#include "core/color.h"
#include "qt-models/diveplannermodel.h"
#include "core/gettextfromc.h"

CylindersModel::CylindersModel(QObject *parent) :
	CleanerTableModel(parent),
	changed(false),
	rows(0)
{
	//	enum {REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, DEPTH, MOD, MND, USE};
	setHeaderDataStrings(QStringList() << "" << tr("Type") << tr("Size") << tr("Work press.") << tr("Start press.") << tr("End press.") << tr("O₂%") << tr("He%")
						 << tr("Deco switch at") <<tr("Bot. MOD") <<tr("MND") << tr("Use"));

}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal && in_planner() && section == WORKINGPRESS)
		return tr("Start press.");
	else
		return CleanerTableModel::headerData(section, orientation, role);
}


CylindersModel *CylindersModel::instance()
{

	static QScopedPointer<CylindersModel> self(new CylindersModel());
	return self.data();
}

static QString get_cylinder_string(cylinder_t *cyl)
{
	QString unit;
	int decimals;
	unsigned int ml = cyl->type.size.mliter;
	pressure_t wp = cyl->type.workingpressure;
	double value;

	// We cannot use "get_volume_units()", because even when
	// using imperial units we may need to show the size in
	// liters: if we don't have a working pressure, we cannot
	// convert the cylinder size to cuft.
	if (wp.mbar && prefs.units.volume == units::CUFT) {
		value = ml_to_cuft(ml) * bar_to_atm(wp.mbar / 1000.0);
		decimals = (value > 20.0) ? 0 : (value > 2.0) ? 1 : 2;
		unit = CylindersModel::tr("cuft");
	} else {
		value = ml / 1000.0;
		decimals = 1;
		unit = CylindersModel::tr("ℓ");
	}

	return QString("%1").arg(value, 0, 'f', decimals) + unit;
}

static QString gas_volume_string(int ml, const char *tail)
{
	double vol;
	const char *unit;
	int decimals;

	vol = get_volume_units(ml, NULL, &unit);
	decimals = (vol > 20.0) ? 0 : (vol > 2.0) ? 1 : 2;

	return QString("%1 %2 %3").arg(vol, 0, 'f', decimals).arg(unit).arg(tail);
}

static QVariant gas_wp_tooltip(cylinder_t *cyl);

static QVariant gas_usage_tooltip(cylinder_t *cyl)
{
	pressure_t startp = cyl->start.mbar ? cyl->start : cyl->sample_start;
	pressure_t endp = cyl->end.mbar ? cyl->end : cyl->sample_end;

	int start, end, used;

	start = gas_volume(cyl, startp);
	end = gas_volume(cyl, endp);
	used = (end && start > end) ? start - end : 0;

	if (!used)
		return gas_wp_tooltip(cyl);

	return gas_volume_string(used, "(") +
		gas_volume_string(start, " -> ") +
		gas_volume_string(end, ")");
}

static QVariant gas_volume_tooltip(cylinder_t *cyl, pressure_t p)
{
	int vol = gas_volume(cyl, p);
	double Z;

	if (!vol)
		return QVariant();

	Z = gas_compressibility_factor(&cyl->gasmix, p.mbar / 1000.0);
	return gas_volume_string(vol, "(Z=") + QString("%1)").arg(Z, 0, 'f', 3);
}

static QVariant gas_wp_tooltip(cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->type.workingpressure);
}

static QVariant gas_start_tooltip(cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->start.mbar ? cyl->start : cyl->sample_start);
}

static QVariant gas_end_tooltip(cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->end.mbar ? cyl->end : cyl->sample_end);
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

	int same_gas = -1;
	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];

	switch (role) {
	case Qt::BackgroundRole: {
		switch (index.column()) {
		// mark the cylinder start / end pressure in red if the values
		// seem implausible
		case START:
		case END:
			pressure_t startp, endp;
			startp = cyl->start.mbar ? cyl->start : cyl->sample_start;
			endp = cyl->end.mbar ? cyl->end : cyl->sample_end;
			if ((startp.mbar && !endp.mbar) ||
					(endp.mbar && startp.mbar <= endp.mbar))
				ret = REDORANGE1_HIGH_TRANS;
			break;
		}
		break;
	}
	case Qt::FontRole: {
		QFont font = defaultModelFont();
		switch (index.column()) {
		// if we don't have manually set pressure data use italic font
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
				ret = get_cylinder_string(cyl);
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
		case MOD:
			if (cyl->bestmix_o2) {
				ret = QString("*");
			} else {
				pressure_t modpO2;
				modpO2.mbar = prefs.bottompo2;
				ret = get_depth_string(gas_mod(&cyl->gasmix, modpO2, &displayed_dive, M_OR_FT(1,1)), true);
			}
			break;
		case MND:
			if (cyl->bestmix_he)
				ret = QString("*");
			else
				ret = get_depth_string(gas_mnd(&cyl->gasmix, prefs.bestmixend, &displayed_dive, M_OR_FT(1,1)), true);
			break;
		case USE:
			ret = gettextFromC::instance()->trGettext(cylinderuse_text[cyl->cylinder_use]);
			break;
		}
		break;
	case Qt::DecorationRole:
	case Qt::SizeHintRole:
		if (index.column() == REMOVE) {
			same_gas = same_gasmix_cylinder(cyl, index.row(), &displayed_dive, false);

			if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!in_planner() && is_cylinder_used(&displayed_dive, index.row()) && same_gas == -1)) {
					ret = trashForbiddenIcon();
			}
			else ret = trashIcon();
		}
		break;

	case Qt::ToolTipRole:
		switch (index.column()) {
		case REMOVE:
			same_gas = same_gasmix_cylinder(cyl, index.row(), &displayed_dive, false);

			if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!in_planner() && is_cylinder_used(&displayed_dive, index.row()) && same_gas == -1)) {
					ret = tr("This gas is in use. Only cylinders that are not used in the dive can be removed.");
			}
			else ret = tr("Clicking here will remove this cylinder.");
			break;
		case TYPE:
		case SIZE:
			return gas_usage_tooltip(cyl);
		case WORKINGPRESS:
			return gas_wp_tooltip(cyl);
		case START:
			return gas_start_tooltip(cyl);
		case END:
			return gas_end_tooltip(cyl);
		case DEPTH:
			ret = tr("Switch depth for deco gas. Calculated using Deco pO₂ preference, unless set manually.");
			break;
		case MOD:
			ret = tr("Calculated using Bottom pO₂ preference. Setting MOD adjusts O₂%, set to '*' for best O₂% for max. depth.");
			break;
		case MND:
			ret = tr("Calculated using Best Mix END preference. Setting MND adjusts He%, set to '*' for best He% for max. depth.");
			break;
		}
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

bool CylindersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString;

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
			// fO2 + fHe must not be greater than 1
			if (get_o2(&cyl->gasmix) + get_he(&cyl->gasmix) > 1000)
				cyl->gasmix.he.permille = 1000 - get_o2(&cyl->gasmix);
			pressure_t modpO2;
			if (displayed_dive.dc.divemode == PSCR)
				modpO2.mbar = prefs.decopo2 + (1000 - get_o2(&cyl->gasmix)) * SURFACE_PRESSURE *
						prefs.o2consumption / prefs.decosac / prefs.pscr_ratio;
			else
				modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&cyl->gasmix, modpO2, &displayed_dive, M_OR_FT(3, 10));
			cyl->bestmix_o2 = false;
			changed = true;
		}
		break;
	case HE:
		if (CHANGED()) {
			cyl->gasmix.he = string_to_fraction(vString.toUtf8().data());
			// fO2 + fHe must not be greater than 1
			if (get_o2(&cyl->gasmix) + get_he(&cyl->gasmix) > 1000)
				cyl->gasmix.o2.permille = 1000 - get_he(&cyl->gasmix);
			cyl->bestmix_he = false;
			changed = true;
		}
		break;
	case DEPTH:
		if (CHANGED()) {
			cyl->depth = string_to_depth(vString.toUtf8().data());
			changed = true;
		}
		break;
	case MOD:
		if (CHANGED()) {
			if (QString::compare(vString.toUtf8().data(), "*") == 0) {
				cyl->bestmix_o2 = true;
				// Calculate fO2 for max. depth
				cyl->gasmix.o2 = best_o2(displayed_dive.maxdepth, &displayed_dive);
			} else {
				cyl->bestmix_o2 = false;
				// Calculate fO2 for input depth
				cyl->gasmix.o2 = best_o2(string_to_depth(vString.toUtf8().data()), &displayed_dive);
			}
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&cyl->gasmix, modpO2, &displayed_dive, M_OR_FT(3, 10));
			changed = true;
		}
		break;
	case MND:
		if (CHANGED()) {
			if (QString::compare(vString.toUtf8().data(), "*") == 0) {
				cyl->bestmix_he = true;
				// Calculate fO2 for max. depth
				cyl->gasmix.he = best_he(displayed_dive.maxdepth, &displayed_dive);
			} else {
				cyl->bestmix_he = false;
				// Calculate fHe for input depth
				cyl->gasmix.he = best_he(string_to_depth(vString.toUtf8().data()), &displayed_dive);
			}
			changed = true;
		}
		break;
	case USE:
		if (CHANGED()) {
			int use = vString.toInt();
			if (use > NUM_GAS_USE - 1 || use < 0)
				use = 0;
			cyl->cylinder_use = (enum cylinderuse)use;
			changed = true;
		}
		break;
	}
	dataChanged(index, index);
	return true;
}

int CylindersModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
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
	emit dataChanged(createIndex(row, 0), createIndex(row, COLUMNS - 1));
}

void CylindersModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

static bool show_cylinder(struct dive *dive, int i)
{
	cylinder_t *cyl = dive->cylinder + i;

	if (is_cylinder_used(dive, i))
		return true;
	if (cylinder_none(cyl))
		return false;
	if (cyl->start.mbar || cyl->sample_start.mbar ||
	    cyl->end.mbar || cyl->sample_end.mbar)
		return true;
	if (cyl->manually_added)
		return true;

	/*
	 * The cylinder has some data, but none of it is very interesting,
	 * it has no pressures and no gas switches. Do we want to show it?
	 */
	return prefs.display_unused_tanks;
}

void CylindersModel::updateDive()
{
	clear();
	rows = 0;
#ifdef DEBUG_CYL
	dump_cylinders(&displayed_dive, true);
#endif
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (show_cylinder(&displayed_dive, i))
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
		if (show_cylinder(d, i))
			rows = i + 1;
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE || index.column() == USE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void CylindersModel::remove(const QModelIndex &index)
{
	int mapping[MAX_CYLINDERS];

	if (index.column() == USE) {
		cylinder_t *cyl = cylinderAt(index);
		if (cyl->cylinder_use == OC_GAS)
			cyl->cylinder_use = NOT_USED;
		else if (cyl->cylinder_use == NOT_USED)
			cyl->cylinder_use = OC_GAS;
		changed = true;
		dataChanged(index, index);
		return;
	}
	if (index.column() != REMOVE) {
		return;
	}
	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];
	int same_gas = same_gasmix_cylinder(cyl, index.row(), &displayed_dive, false);

	if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
		(!in_planner() && is_cylinder_used(&displayed_dive, index.row()) && same_gas == -1))
			return;

	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	// if we didn't find an identical gas, point same_gas at the index.row()
	if (same_gas == -1)
		same_gas = index.row();
	if (index.row() == 0) {
		// first gas - we need to make sure that the same gas ends up
		// as first gas
		memmove(cyl, &displayed_dive.cylinder[same_gas], sizeof(*cyl));
		remove_cylinder(&displayed_dive, same_gas);
		for (int i = 0; i < same_gas - 1; i++)
			mapping[i] = i;
		mapping[same_gas] = 0;
		for (int i = same_gas + 1; i < MAX_CYLINDERS; i++)
			mapping[i] = i - 1;
	} else {
		remove_cylinder(&displayed_dive, index.row());
		if (same_gas > index.row())
			same_gas--;
		for (int i = 0; i < index.row(); i++)
			mapping[i] = i;
		mapping[index.row()] = same_gas;
		for (int i = index.row() + 1; i < MAX_CYLINDERS; i++)
			mapping[i] = i - 1;
	}
	cylinder_renumber(&displayed_dive, mapping);
	if (in_planner())
		DivePlannerPointsModel::instance()->cylinderRenumber(mapping);
	changed = true;
	endRemoveRows();
	dataChanged(index, index);
}

void CylindersModel::moveAtFirst(int cylid)
{
	int mapping[MAX_CYLINDERS];
	cylinder_t temp_cyl;

	beginMoveRows(QModelIndex(), cylid, cylid, QModelIndex(), 0);
	memmove(&temp_cyl, &displayed_dive.cylinder[cylid], sizeof(temp_cyl));
	for (int i = cylid - 1; i >= 0; i--) {
		memmove(&displayed_dive.cylinder[i + 1], &displayed_dive.cylinder[i], sizeof(temp_cyl));
		mapping[i] = i + 1;
	}
	memmove(&displayed_dive.cylinder[0], &temp_cyl, sizeof(temp_cyl));
	mapping[cylid] = 0;
	for (int i = cylid + 1; i < MAX_CYLINDERS; i++)
		mapping[i] = i;
	cylinder_renumber(&displayed_dive, mapping);
	if (in_planner())
		DivePlannerPointsModel::instance()->cylinderRenumber(mapping);
	changed = true;
	endMoveRows();
}

void CylindersModel::updateDecoDepths(pressure_t olddecopo2)
{
	pressure_t decopo2;
	decopo2.mbar = prefs.decopo2;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		/* If the gas's deco MOD matches the old pO2, it will have been automatically calculated and should be updated.
		 * If they don't match, we should leave the user entered depth as it is */
		if (cyl->depth.mm == gas_mod(&cyl->gasmix, olddecopo2, &displayed_dive, M_OR_FT(3, 10)).mm) {
			cyl->depth = gas_mod(&cyl->gasmix, decopo2, &displayed_dive, M_OR_FT(3, 10));
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(MAX_CYLINDERS - 1, COLUMNS - 1));
}

void CylindersModel::updateTrashIcon()
{
	emit dataChanged(createIndex(0, 0), createIndex(MAX_CYLINDERS - 1, 0));
}

bool CylindersModel::updateBestMixes()
{
	// Check if any of the cylinders are best mixes, update if needed
	bool gasUpdated = false;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		if (cyl->bestmix_o2) {
			cyl->gasmix.o2 = best_o2(displayed_dive.maxdepth, &displayed_dive);
			// fO2 + fHe must not be greater than 1
			if (get_o2(&cyl->gasmix) + get_he(&cyl->gasmix) > 1000)
				cyl->gasmix.he.permille = 1000 - get_o2(&cyl->gasmix);
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&cyl->gasmix, modpO2, &displayed_dive, M_OR_FT(3, 10));
			gasUpdated = true;
		}
		if (cyl->bestmix_he) {
			cyl->gasmix.he = best_he(displayed_dive.maxdepth, &displayed_dive);
			// fO2 + fHe must not be greater than 1
			if (get_o2(&cyl->gasmix) + get_he(&cyl->gasmix) > 1000)
				cyl->gasmix.o2.permille = 1000 - get_he(&cyl->gasmix);
			gasUpdated = true;
		}
	}
	/* This slot is called when the bottom pO2 and END preferences are updated, we want to
	 * emit dataChanged so MOD and MND are refreshed, even if the gas mix hasn't been changed */
	if (gasUpdated)
		emit dataChanged(createIndex(0, 0), createIndex(MAX_CYLINDERS - 1, COLUMNS - 1));
	return gasUpdated;
}
