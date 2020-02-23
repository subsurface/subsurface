// SPDX-License-Identifier: GPL-2.0
#include "cylindermodel.h"
#include "tankinfomodel.h"
#include "models.h"
#include "core/qthelper.h"
#include "core/divelist.h" // for mark_divelist_changed()
#include "core/color.h"
#include "qt-models/diveplannermodel.h"
#include "core/gettextfromc.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/subsurface-string.h"

CylindersModel::CylindersModel(QObject *parent) : CleanerTableModel(parent),
	changed(false),
	d(nullptr),
	rows(0)
{
	//	enum {REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, DEPTH, MOD, MND, USE, IS_USED};
	setHeaderDataStrings(QStringList() << "" << tr("Type") << tr("Size") << tr("Work press.") << tr("Start press.") << tr("End press.") << tr("O₂%") << tr("He%")
						 << tr("Deco switch at") <<tr("Bot. MOD") <<tr("MND") << tr("Use"));

	connect(&diveListNotifier, &DiveListNotifier::cylindersReset, this, &CylindersModel::cylindersReset);
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal && in_planner() && section == WORKINGPRESS)
		return tr("Start press.");
	else
		return CleanerTableModel::headerData(section, orientation, role);
}

static QString get_cylinder_string(const cylinder_t *cyl)
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

	return QString("%L1").arg(value, 0, 'f', decimals) + unit;
}

static QString gas_volume_string(int ml, const char *tail)
{
	double vol;
	const char *unit;
	int decimals;

	vol = get_volume_units(ml, NULL, &unit);
	decimals = (vol > 20.0) ? 0 : (vol > 2.0) ? 1 : 2;

	return QString("%L1 %2 %3").arg(vol, 0, 'f', decimals).arg(unit).arg(tail);
}

static QVariant gas_wp_tooltip(const cylinder_t *cyl);

static QVariant gas_usage_tooltip(const cylinder_t *cyl)
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

static QVariant gas_volume_tooltip(const cylinder_t *cyl, pressure_t p)
{
	int vol = gas_volume(cyl, p);
	double Z;

	if (!vol)
		return QVariant();

	Z = gas_compressibility_factor(cyl->gasmix, p.mbar / 1000.0);
	return gas_volume_string(vol, "(Z=") + QString("%1)").arg(Z, 0, 'f', 3);
}

static QVariant gas_wp_tooltip(const cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->type.workingpressure);
}

static QVariant gas_start_tooltip(const cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->start.mbar ? cyl->start : cyl->sample_start);
}

static QVariant gas_end_tooltip(const cylinder_t *cyl)
{
	return gas_volume_tooltip(cyl, cyl->end.mbar ? cyl->end : cyl->sample_end);
}

static QVariant percent_string(fraction_t fraction)
{
	int permille = fraction.permille;

	if (!permille)
		return QVariant();
	return QString("%L1%").arg(permille / 10.0, 0, 'f', 1);
}

bool CylindersModel::cylinderUsed(int i) const
{
	if (i < 0 || i >= d->cylinders.nr)
		return false;
	if (is_cylinder_used(d, i))
		return true;

	cylinder_t *cyl = get_cylinder(d, i);
	if (cyl->start.mbar || cyl->sample_start.mbar ||
	    cyl->end.mbar || cyl->sample_end.mbar)
		return true;
	if (cyl->manually_added)
		return true;

	/*
	 * The cylinder has some data, but none of it is very interesting,
	 * it has no pressures and no gas switches. Do we want to show it?
	 */
	return false;
}

QVariant CylindersModel::data(const QModelIndex &index, int role) const
{
	if (!d || !index.isValid() || index.row() >= rows)
		return QVariant();

	if (index.row() >= d->cylinders.nr) {
		qWarning("CylindersModel and dive are out of sync!");
		return QVariant();
	}

	const cylinder_t *cyl = get_cylinder(d, index.row());

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
				return REDORANGE1_HIGH_TRANS;
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
		return font;
	}
	case Qt::TextAlignmentRole:
		return Qt::AlignCenter;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			return QString(cyl->type.description);
		case SIZE:
			if (cyl->type.size.mliter)
				return get_cylinder_string(cyl);
			break;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar)
				return get_pressure_string(cyl->type.workingpressure, true);
			break;
		case START:
			if (cyl->start.mbar)
				return get_pressure_string(cyl->start, true);
			else if (cyl->sample_start.mbar)
				return get_pressure_string(cyl->sample_start, true);
			break;
		case END:
			if (cyl->end.mbar)
				return get_pressure_string(cyl->end, true);
			else if (cyl->sample_end.mbar)
				return get_pressure_string(cyl->sample_end, true);
			break;
		case O2:
			return percent_string(cyl->gasmix.o2);
		case HE:
			return percent_string(cyl->gasmix.he);
		case DEPTH:
			return get_depth_string(cyl->depth, true);
		case MOD:
			if (cyl->bestmix_o2) {
				return QStringLiteral("*");
			} else {
				pressure_t modpO2;
				modpO2.mbar = prefs.bottompo2;
				return get_depth_string(gas_mod(cyl->gasmix, modpO2, d, M_OR_FT(1,1)), true);
			}
		case MND:
			if (cyl->bestmix_he)
				return QStringLiteral("*");
			else
				return get_depth_string(gas_mnd(cyl->gasmix, prefs.bestmixend, d, M_OR_FT(1,1)), true);
			break;
		case USE:
			return gettextFromC::tr(cylinderuse_text[cyl->cylinder_use]);
		case WORKINGPRESS_INT:
			return static_cast<int>(cyl->type.workingpressure.mbar);
		case SIZE_INT:
			return static_cast<int>(cyl->type.size.mliter);
		}
		break;
	case Qt::DecorationRole:
	case Qt::SizeHintRole:
		if (index.column() == REMOVE) {
			if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!in_planner() && is_cylinder_prot(d, index.row()))) {
					return trashForbiddenIcon();
			}
			return trashIcon();
		}
		break;
	case Qt::ToolTipRole:
		switch (index.column()) {
		case REMOVE:
			if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!in_planner() && is_cylinder_prot(d, index.row()))) {
					return tr("This gas is in use. Only cylinders that are not used in the dive can be removed.");
			}
			return tr("Clicking here will remove this cylinder.");
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
			return tr("Switch depth for deco gas. Calculated using Deco pO₂ preference, unless set manually.");
		case MOD:
			return tr("Calculated using Bottom pO₂ preference. Setting MOD adjusts O₂%, set to '*' for best O₂% for max. depth.");
		case MND:
			return tr("Calculated using Best Mix END preference. Setting MND adjusts He%, set to '*' for best He% for max. depth.");
		}
		break;
	}

	return QVariant();
}

cylinder_t *CylindersModel::cylinderAt(const QModelIndex &index)
{
	if (!d)
		return nullptr;
	return get_cylinder(d, index.row());
}

bool CylindersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString;

	if (!d)
		return false;

	cylinder_t *cyl = cylinderAt(index);
	if (!cyl)
		return false;

	if (role == PASS_IN_ROLE) {
		// this is our magic 'pass data in' function that allows the delegate to get
		// the data here without silly unit conversions;
		// so we only implement the two columns we care about
		switch (index.column()) {
		case SIZE:
			if (cyl->type.size.mliter != value.toInt()) {
				cyl->type.size.mliter = value.toInt();
				dataChanged(index, index);
			}
			return true;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar != value.toInt()) {
				cyl->type.workingpressure.mbar = value.toInt();
				dataChanged(index, index);
			}
			return true;
		}
		return false;
	}

	switch (index.column()) {
	case TYPE: {
			QString type = value.toString();
			if (!same_string(qPrintable(type), cyl->type.description)) {
				free((void *)cyl->type.description);
				cyl->type.description = strdup(qPrintable(type));
				changed = true;
			}
		}
		break;
	case SIZE:
		if (CHANGED()) {
			TankInfoModel *tanks = TankInfoModel::instance();
			QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, cyl->type.description);

			cyl->type.size = string_to_volume(qPrintable(vString), cyl->type.workingpressure);
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
			cyl->type.workingpressure = string_to_pressure(qPrintable(vString));
			if (!matches.isEmpty())
				tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
			changed = true;
		}
		break;
	case START:
		if (CHANGED()) {
			cyl->start = string_to_pressure(qPrintable(vString));
			changed = true;
		}
		break;
	case END:
		if (CHANGED()) {
			//&& (!cyl->start.mbar || string_to_pressure(qPrintable(vString)).mbar <= cyl->start.mbar)) {
			cyl->end = string_to_pressure(qPrintable(vString));
			changed = true;
		}
		break;
	case O2:
		if (CHANGED()) {
			cyl->gasmix.o2 = string_to_fraction(qPrintable(vString));
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl->gasmix) + get_he(cyl->gasmix) > 1000)
				cyl->gasmix.he.permille = 1000 - get_o2(cyl->gasmix);
			pressure_t modpO2;
			if (d->dc.divemode == PSCR)
				modpO2.mbar = prefs.decopo2 + (1000 - get_o2(cyl->gasmix)) * SURFACE_PRESSURE *
						prefs.o2consumption / prefs.decosac / prefs.pscr_ratio;
			else
				modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(cyl->gasmix, modpO2, d, M_OR_FT(3, 10));
			cyl->bestmix_o2 = false;
			changed = true;
		}
		break;
	case HE:
		if (CHANGED()) {
			cyl->gasmix.he = string_to_fraction(qPrintable(vString));
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl->gasmix) + get_he(cyl->gasmix) > 1000)
				cyl->gasmix.o2.permille = 1000 - get_he(cyl->gasmix);
			cyl->bestmix_he = false;
			changed = true;
		}
		break;
	case DEPTH:
		if (CHANGED()) {
			cyl->depth = string_to_depth(qPrintable(vString));
			changed = true;
		}
		break;
	case MOD:
		if (CHANGED()) {
			if (QString::compare(qPrintable(vString), "*") == 0) {
				cyl->bestmix_o2 = true;
				// Calculate fO2 for max. depth
				cyl->gasmix.o2 = best_o2(d->maxdepth, d);
			} else {
				cyl->bestmix_o2 = false;
				// Calculate fO2 for input depth
				cyl->gasmix.o2 = best_o2(string_to_depth(qPrintable(vString)), d);
			}
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(cyl->gasmix, modpO2, d, M_OR_FT(3, 10));
			changed = true;
		}
		break;
	case MND:
		if (CHANGED()) {
			if (QString::compare(qPrintable(vString), "*") == 0) {
				cyl->bestmix_he = true;
				// Calculate fO2 for max. depth
				cyl->gasmix.he = best_he(d->maxdepth, d, prefs.o2narcotic, cyl->gasmix.o2);
			} else {
				cyl->bestmix_he = false;
				// Calculate fHe for input depth
				cyl->gasmix.he = best_he(string_to_depth(qPrintable(vString)), d, prefs.o2narcotic, cyl->gasmix.o2);
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

int CylindersModel::rowCount(const QModelIndex&) const
{
	return rows;
}

void CylindersModel::add()
{
	if (!d)
		return;
	int row = rows;
	cylinder_t cyl = create_new_cylinder(d);
	beginInsertRows(QModelIndex(), row, row);
	add_to_cylinder_table(&d->cylinders, row, cyl);
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

void CylindersModel::updateDive(dive *dIn)
{
#ifdef DEBUG_CYL
	if (d)
		dump_cylinders(dIn, true);
#endif
	beginResetModel();
	d = dIn;
	rows = d ? d->cylinders.nr : 0;
	endResetModel();
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE || index.column() == USE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void CylindersModel::remove(QModelIndex index)
{
	if (!d)
		return;
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

	if (index.column() != REMOVE)
		return;

	if ((in_planner() && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
		(!in_planner() && is_cylinder_prot(d, index.row())))
			return;

	beginRemoveRows(QModelIndex(), index.row(), index.row());
	rows--;
	remove_cylinder(d, index.row());
	changed = true;
	endRemoveRows();

	// Create a mapping of cylinder indices:
	// 1) Fill mapping[0]..mapping[index-1] with 0..index
	// 2) Set mapping[index] to -1
	// 3) Fill mapping[index+1]..mapping[end] with index..
	std::vector<int> mapping(d->cylinders.nr + 1);
	std::iota(mapping.begin(), mapping.begin() + index.row(), 0);
	mapping[index.row()] = -1;
	std::iota(mapping.begin() + index.row() + 1, mapping.end(), index.row());

	cylinder_renumber(d, &mapping[0]);
	if (in_planner())
		DivePlannerPointsModel::instance()->cylinderRenumber(&mapping[0]);
	changed = true;
}

void CylindersModel::moveAtFirst(int cylid)
{
	if (!d)
		return;

	cylinder_t temp_cyl;

	beginMoveRows(QModelIndex(), cylid, cylid, QModelIndex(), 0);
	memmove(&temp_cyl, get_cylinder(d, cylid), sizeof(temp_cyl));
	for (int i = cylid - 1; i >= 0; i--)
		memmove(get_cylinder(d, i + 1), get_cylinder(d, i), sizeof(temp_cyl));
	memmove(get_cylinder(d, 0), &temp_cyl, sizeof(temp_cyl));

	// Create a mapping of cylinder indices:
	// 1) Fill mapping[0]..mapping[cyl] with 0..index
	// 2) Set mapping[cyl] to 0
	// 3) Fill mapping[cyl+1]..mapping[end] with cyl..
	std::vector<int> mapping(d->cylinders.nr);
	std::iota(mapping.begin(), mapping.begin() + cylid, 1);
	mapping[cylid] = 0;
	std::iota(mapping.begin() + (cylid + 1), mapping.end(), cylid);
	cylinder_renumber(d, &mapping[0]);
	if (in_planner())
		DivePlannerPointsModel::instance()->cylinderRenumber(&mapping[0]);
	changed = true;
	endMoveRows();
}

void CylindersModel::updateDecoDepths(pressure_t olddecopo2)
{
	if (!d)
		return;

	pressure_t decopo2;
	decopo2.mbar = prefs.decopo2;
	for (int i = 0; i < d->cylinders.nr; i++) {
		cylinder_t *cyl = get_cylinder(d, i);
		/* If the gas's deco MOD matches the old pO2, it will have been automatically calculated and should be updated.
		 * If they don't match, we should leave the user entered depth as it is */
		if (cyl->depth.mm == gas_mod(cyl->gasmix, olddecopo2, d, M_OR_FT(3, 10)).mm) {
			cyl->depth = gas_mod(cyl->gasmix, decopo2, d, M_OR_FT(3, 10));
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(d->cylinders.nr - 1, COLUMNS - 1));
}

void CylindersModel::updateTrashIcon()
{
	if (!d)
		return;

	emit dataChanged(createIndex(0, 0), createIndex(d->cylinders.nr - 1, 0));
}

bool CylindersModel::updateBestMixes()
{
	if (!d)
		return false;

	// Check if any of the cylinders are best mixes, update if needed
	bool gasUpdated = false;
	for (int i = 0; i < d->cylinders.nr; i++) {
		cylinder_t *cyl = get_cylinder(d, i);
		if (cyl->bestmix_o2) {
			cyl->gasmix.o2 = best_o2(d->maxdepth, d);
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl->gasmix) + get_he(cyl->gasmix) > 1000)
				cyl->gasmix.he.permille = 1000 - get_o2(cyl->gasmix);
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(cyl->gasmix, modpO2, d, M_OR_FT(3, 10));
			gasUpdated = true;
		}
		if (cyl->bestmix_he) {
			cyl->gasmix.he = best_he(d->maxdepth, d, prefs.o2narcotic, cyl->gasmix.o2);
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl->gasmix) + get_he(cyl->gasmix) > 1000)
				cyl->gasmix.o2.permille = 1000 - get_he(cyl->gasmix);
			gasUpdated = true;
		}
	}
	/* This slot is called when the bottom pO2 and END preferences are updated, we want to
	 * emit dataChanged so MOD and MND are refreshed, even if the gas mix hasn't been changed */
	if (gasUpdated)
		emit dataChanged(createIndex(0, 0), createIndex(d->cylinders.nr - 1, COLUMNS - 1));
	return gasUpdated;
}

void CylindersModel::cylindersReset(const QVector<dive *> &dives)
{
	// This model only concerns the currently displayed dive. If this is not among the
	// dives that had their cylinders reset, exit.
	if (!d || std::find(dives.begin(), dives.end(), d) == dives.end())
		return;

	// And update the model (the actual change was already performed in the backend)..
	beginResetModel();
	rows = d->cylinders.nr;
	endResetModel();
}

CylindersModelFiltered::CylindersModelFiltered(QObject *parent) : QSortFilterProxyModel(parent)
{
	setSourceModel(&source);
}

void CylindersModelFiltered::updateDive(dive *d)
{
	source.updateDive(d);
}

void CylindersModelFiltered::clear()
{
	source.clear();
}

void CylindersModelFiltered::add()
{
	source.add();
}

CylindersModel *CylindersModelFiltered::model()
{
	return &source;
}

void CylindersModelFiltered::remove(QModelIndex index)
{
	source.remove(mapToSource(index));
}

bool CylindersModelFiltered::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	return prefs.display_unused_tanks || source.cylinderUsed(source_row);
}
