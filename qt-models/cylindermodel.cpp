// SPDX-License-Identifier: GPL-2.0
#include "cylindermodel.h"
#include "models.h"
#include "commands/command.h"
#include "core/qthelper.h"
#include "core/color.h"
#include "qt-models/diveplannermodel.h"
#include "core/gettextfromc.h"
#include "core/range.h"
#include "core/sample.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/subsurface-string.h"
#include "core/tanksensormapping.h"
#include <string>

CylindersModel::CylindersModel(bool planner, QObject *parent) : CleanerTableModel(parent),
	d(nullptr),
	dcNr(-1),
	inPlanner(planner),
	numRows(0),
	tempRow(-1)
{
	setHeaderDataStrings(QStringList() << "#" << "" << tr("Type") << tr("Size") << tr("Work press.") << tr("Start press.") << tr("End press.") << tr("O₂%") << tr("He%")
					   << tr("Deco switch at") << tr("Bot. MOD") << tr("MND") << tr("Use") << ""
					   << "" << tr("Sensor"));

	connect(&diveListNotifier, &DiveListNotifier::cylindersReset, this, &CylindersModel::cylindersReset);
	connect(&diveListNotifier, &DiveListNotifier::cylinderAdded, this, &CylindersModel::cylinderAdded);
	connect(&diveListNotifier, &DiveListNotifier::cylinderRemoved, this, &CylindersModel::cylinderRemoved);
	connect(&diveListNotifier, &DiveListNotifier::cylinderEdited, this, &CylindersModel::cylinderEdited);
}

QVariant CylindersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal && inPlanner && section == WORKINGPRESS)
		return tr("Start press.");
	else if (role == Qt::SizeHintRole && orientation == Qt::Horizontal && section == REMOVE)
		return QSize(0, 0);
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
		unit = CylindersModel::tr("L");
	}

	return QString("%L1").arg(value, 0, 'f', decimals) + unit;
}

static QString gas_volume_string(volume_t volume, const char *tail)
{
	double vol;
	const char *unit;
	int decimals;

	vol = get_volume_units(volume.mliter, NULL, &unit);
	decimals = (vol > 20.0) ? 0 : (vol > 2.0) ? 1 : 2;

	return QString("%L1 %2 %3").arg(vol, 0, 'f', decimals).arg(unit).arg(tail);
}

static QVariant gas_wp_tooltip(const cylinder_t *cyl);

static QVariant gas_usage_tooltip(const cylinder_t *cyl)
{
	pressure_t startp = cyl->start.mbar ? cyl->start : cyl->sample_start;
	pressure_t endp = cyl->end.mbar ? cyl->end : cyl->sample_end;

	volume_t start = cyl->gas_volume(startp);
	volume_t end = cyl->gas_volume(endp);
	// TOOO: implement comparison and subtraction on units.h types.
	volume_t used = (end.mliter && start.mliter > end.mliter) ? volume_t { .mliter = start.mliter - end.mliter } : volume_t();

	if (!used.mliter)
		return gas_wp_tooltip(cyl);

	return gas_volume_string(used, "(") +
		gas_volume_string(start, " -> ") +
		gas_volume_string(end, ")");
}

static QVariant gas_volume_tooltip(const cylinder_t *cyl, pressure_t p)
{
	volume_t vol = cyl->gas_volume(p);
	double Z;

	if (!vol.mliter)
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

// Calculate the number of displayed cylinders: If we are in planner
// or prefs.include_unused_tanks is set we show unused cylinders
// at the end of the list.
int CylindersModel::calcNumRows() const
{
	if (!d)
		return 0;
	if (inPlanner || prefs.include_unused_tanks)
		return static_cast<int>(d->cylinders.size());
	return first_hidden_cylinder(d);
}

QVariant CylindersModel::data(const QModelIndex &index, int role) const
{
	if (!d || !index.isValid())
		return QVariant();

	if (index.row() >= numRows) {
		qWarning("CylindersModel and dive are out of sync!");
		return QVariant();
	}

	const cylinder_t *cyl = index.row() == tempRow ? &tempCyl : d->get_cylinder(index.row());

	bool isInappropriateUse = !is_cylinder_use_appropriate(*d->get_dc(dcNr), *cyl, true);
	switch (role) {
	case Qt::BackgroundRole:
		switch (index.column()) {
		// mark the cylinder start / end pressure in red if the values
		// seem implausible
		case START:
		case END:
			{
				pressure_t startp = cyl->start.mbar ? cyl->start : cyl->sample_start;
				pressure_t endp = cyl->end.mbar ? cyl->end : cyl->sample_end;
				if ((startp.mbar && !endp.mbar) ||
						(endp.mbar && startp.mbar <= endp.mbar))
					return REDORANGE1_HIGH_TRANS;
			}

			break;
		case USE:
			if (isInappropriateUse)
				return REDORANGE1_HIGH_TRANS;

			break;
		}

		break;
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

		font.setItalic(isInappropriateUse);

		return font;
	}
	case Qt::TextAlignmentRole:
		return Qt::AlignCenter;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case NUMBER:
			return index.row() + 1;
		case TYPE:
			return QString::fromStdString(cyl->type.description);
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
				modpO2.mbar = inPlanner ? prefs.bottompo2 : (int)(prefs.modpO2 * 1000.0);
				return get_depth_string(d->gas_mod(cyl->gasmix, modpO2, m_or_ft(1, 1)), true);
			}
		case MND:
			if (cyl->bestmix_he)
				return QStringLiteral("*");
			else
				return get_depth_string(d->gas_mnd(cyl->gasmix, prefs.bestmixend, m_or_ft(1, 1)), true);
			break;
		case USE:
			return gettextFromC::tr(cylinderuse_text[cyl->cylinder_use]);
		case WORKINGPRESS_INT:
			return static_cast<int>(cyl->type.workingpressure.mbar);
		case SIZE_INT:
			return static_cast<int>(cyl->type.size.mliter);
		case SENSORS: {
			for (const auto &mapping: d->get_dc(dcNr)->tank_sensor_mappings)
				if ((int)mapping.cylinder_index == index.row())
					return QString::number(mapping.sensor_id + 1);

			return QString();
		}
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE) {
			if ((inPlanner && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!inPlanner && d->is_cylinder_prot(index.row()))) {
					return trashForbiddenIcon();
			}
			return trashIcon();
		}
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			return QSize(0, 0);
		break;
	case Qt::ToolTipRole:
		switch (index.column()) {
		case REMOVE:
			if ((inPlanner && DivePlannerPointsModel::instance()->tankInUse(index.row())) ||
				(!inPlanner && d->is_cylinder_prot(index.row()))) {
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
		case SENSORS:
			return tr("Select the tank pressure sensor id for this cylinder.");
		}
		break;
	}

	return QVariant();
}

cylinder_t *CylindersModel::cylinderAt(const QModelIndex &index)
{
	if (!d)
		return nullptr;
	return d->get_cylinder(index.row());
}

bool CylindersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!d)
		return false;

	int row = index.row();
	if (row < 0 || row >= numRows)
		return false;

	if (index.column() == NUMBER)
		return true;

	// Here we handle a few cases that allow us to set / commit / revert
	// a temporary row. This is a horribly misuse of the model/view system.
	// The reason it is done this way is that the planner and the equipment
	// tab use different model-classes which are not in a superclass / subclass
	// relationship.
	switch (role) {
	case TEMP_ROLE:
		// TEMP_ROLE means that we are not supposed to write through to the
		// actual dive, but a temporary cylinder that is displayed while the
		// user browses throught the cylinder types.
		initTempCyl(index.row());

		switch (index.column()) {
		case TYPE: {
			std::string type = value.toString().toStdString();
			if (type != tempCyl.type.description) {
				tempCyl.type.description = type;
				dataChanged(index, index);
			}
			return true;
		}
		case SIZE:
			if (tempCyl.type.size.mliter != value.toInt()) {
				tempCyl.type.size.mliter = value.toInt();
				dataChanged(index, index);
			}
			return true;
		case WORKINGPRESS:
			if (tempCyl.type.workingpressure.mbar != value.toInt()) {
				tempCyl.type.workingpressure.mbar = value.toInt();
				dataChanged(index, index);
			}
			return true;
		}
		return false;
	case COMMIT_ROLE:
		commitTempCyl(row);
		return true;
	case REVERT_ROLE:
		clearTempCyl();
		return true;
	default:
		break;
	}

	QString vString = value.toString();
	bool changed = vString != data(index, role).toString();

	// First, we make a shallow copy of the old cylinder. Then we modify the fields inside that copy.
	// At the end, we either place an EditCylinder undo command (EquipmentTab) or copy the cylinder back (planner).
	// Yes, this is not ideal, but the pragmatic thing to do for now.
	cylinder_t cyl = *d->get_cylinder(row);

	if (index.column() != TYPE && !changed)
		return false;

	Command::EditCylinderType type = Command::EditCylinderType::TYPE;
	switch (index.column()) {
	case TYPE:
		cyl.type.description = vString.toStdString();
		type = Command::EditCylinderType::TYPE;
		break;
	case SIZE:
		cyl.type.size = string_to_volume(qPrintable(vString), cyl.type.workingpressure);
		type = Command::EditCylinderType::TYPE;
		break;
	case WORKINGPRESS:
		cyl.type.workingpressure = string_to_pressure(qPrintable(vString));
		type = Command::EditCylinderType::TYPE;
		break;
	case START:
		cyl.start = string_to_pressure(qPrintable(vString));
		type = Command::EditCylinderType::PRESSURE;
		break;
	case END:
		//if (!cyl->start.mbar || string_to_pressure(qPrintable(vString)).mbar <= cyl->start.mbar) {
		cyl.end = string_to_pressure(qPrintable(vString));
		type = Command::EditCylinderType::PRESSURE;
		break;
	case O2: {
			cyl.gasmix.o2 = string_to_fraction(qPrintable(vString));
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl.gasmix) + get_he(cyl.gasmix) > 1000)
				cyl.gasmix.he.permille = 1000 - get_o2(cyl.gasmix);
			pressure_t modpO2;
			if (d->dcs[0].divemode == PSCR)
				modpO2.mbar = prefs.decopo2 + (1000 - get_o2(cyl.gasmix)) * (1_atm).mbar *
						prefs.o2consumption / prefs.decosac / prefs.pscr_ratio;
			else
				modpO2.mbar = prefs.decopo2;
			cyl.depth = d->gas_mod(cyl.gasmix, modpO2, m_or_ft(3, 10));
			cyl.bestmix_o2 = false;
		}
		type = Command::EditCylinderType::GASMIX;
		break;
	case HE:
		cyl.gasmix.he = string_to_fraction(qPrintable(vString));
		// fO2 + fHe must not be greater than 1
		if (get_o2(cyl.gasmix) + get_he(cyl.gasmix) > 1000)
			cyl.gasmix.o2.permille = 1000 - get_he(cyl.gasmix);
		cyl.bestmix_he = false;
		type = Command::EditCylinderType::GASMIX;
		break;
	case DEPTH:
		cyl.depth = string_to_depth(qPrintable(vString));
		type = Command::EditCylinderType::GASMIX;
		break;
	case MOD: {
			if (QString::compare(qPrintable(vString), "*") == 0) {
				cyl.bestmix_o2 = true;
				// Calculate fO2 for max. depth
				cyl.gasmix.o2 = d->best_o2(d->maxdepth, inPlanner);
			} else {
				cyl.bestmix_o2 = false;
				// Calculate fO2 for input depth
				cyl.gasmix.o2 = d->best_o2(string_to_depth(qPrintable(vString)), inPlanner);
			}
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl.depth = d->gas_mod(cyl.gasmix, modpO2, m_or_ft(3, 10));
		}
		type = Command::EditCylinderType::GASMIX;
		break;
	case MND:
		if (QString::compare(qPrintable(vString), "*") == 0) {
			cyl.bestmix_he = true;
			// Calculate fO2 for max. depth
			cyl.gasmix.he = d->best_he(d->maxdepth, prefs.o2narcotic, make_fraction(get_o2(cyl.gasmix)));
		} else {
			cyl.bestmix_he = false;
			// Calculate fHe for input depth
			cyl.gasmix.he = d->best_he(string_to_depth(qPrintable(vString)), prefs.o2narcotic, make_fraction(get_o2(cyl.gasmix)));
		}
		type = Command::EditCylinderType::GASMIX;
		break;
	case USE: {
			int use = vString.toInt();
			if (use > NUM_GAS_USE - 1 || use < 0)
				use = 0;
			cyl.cylinder_use = (enum cylinderuse)use;
		}
		type = Command::EditCylinderType::TYPE;
		break;
	case SENSORS: {
		bool ok = false;
		int s = vString.toInt(&ok);
		const struct divecomputer &dc = *d->get_dc(dcNr);
		if (ok && std::find_if(dc.tank_sensor_mappings.begin(), dc.tank_sensor_mappings.end(), [row, s](const auto &mapping) { return (const int)mapping.cylinder_index == row && mapping.sensor_id == s - 1; }) == dc.tank_sensor_mappings.end()) {
			Command::editSensors(row, s, dcNr);
			// We don't use the edit cylinder command and editing sensors is not relevant for planner
			return true;
		}
		return false;
	}
	}

	if (inPlanner) {
		// In the planner - simply overwrite the cylinder in the dive with the modified cylinder.
		*d->get_cylinder(row) = std::move(cyl);
		dataChanged(index, index);
	} else {
		// On the EquipmentTab - place an editCylinder command.
		int count = Command::editCylinder(index.row(), std::move(cyl), type, false);
		emit divesEdited(count);
	}
	return true;
}

int CylindersModel::rowCount(const QModelIndex&) const
{
	return numRows;
}

// Only invoked from planner.
void CylindersModel::add()
{
	if (!d)
		return;
	int row = static_cast<int>(d->cylinders.size());
	cylinder_t cyl = create_new_manual_cylinder(d);
	beginInsertRows(QModelIndex(), row, row);
	d->cylinders.add(row, std::move(cyl));
	++numRows;
	endInsertRows();
	emit dataChanged(createIndex(row, 0), createIndex(row, COLUMNS - 1));
}

void CylindersModel::clear()
{
	beginResetModel();
	d = nullptr;
	dcNr = -1;
	endResetModel();
}

void CylindersModel::updateDive(dive *dIn, int dcNrIn)
{
#ifdef DEBUG_CYL
	if (d)
		dump_cylinders(dIn, true);
#endif
	beginResetModel();
	d = dIn;
	dcNr = dcNrIn;
	numRows = calcNumRows();
	endResetModel();
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex &index) const
{
	if (index.column() == NUMBER || index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

// This function is only invoked from the planner! Therefore, there is
// no need to check whether we are in the planner. Perhaps move some
// of this functionality to the planner itself.
void CylindersModel::remove(QModelIndex index)
{
	if (!d)
		return;

	if (index.column() != REMOVE)
		return;

	if (DivePlannerPointsModel::instance()->tankInUse(index.row()))
			return;

	beginRemoveRows(QModelIndex(), index.row(), index.row());
	remove_cylinder(d, index.row());
	--numRows;
	endRemoveRows();

	std::vector<int> mapping = get_cylinder_map_for_remove(static_cast<int>(d->cylinders.size() + 1), index.row());
	cylinder_renumber(*d, mapping.data());
	DivePlannerPointsModel::instance()->cylinderRenumber(mapping.data());
}

void CylindersModel::cylinderAdded(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	// The row was already inserted by the undo command. Just inform the model.
	if (pos < numRows) {
		beginInsertRows(QModelIndex(), pos, pos);
		++numRows;
		endInsertRows();
	}
	updateNumRows();
}

void CylindersModel::cylinderRemoved(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	// The row was already deleted by the undo command. Just inform the model.
	if (pos < numRows) {
		beginRemoveRows(QModelIndex(), pos, pos);
		--numRows;
		endRemoveRows();
	}
	updateNumRows();
}

void CylindersModel::cylinderEdited(struct dive *changed, int pos)
{
	if (d != changed)
		return;

	if (pos < numRows)
		dataChanged(index(pos, TYPE), index(pos, USE));
	updateNumRows();
}

void CylindersModel::updateNumRows()
{
	int numRowsNew = calcNumRows();
	if (numRowsNew < numRows) {
		beginRemoveRows(QModelIndex(), numRowsNew, numRows - 1);
		numRows = numRowsNew;
		endRemoveRows();
	} else if (numRowsNew > numRows) {
		beginInsertRows(QModelIndex(), numRows, numRowsNew - 1);
		numRows = numRowsNew;
		endInsertRows();
	}
}

// Only invoked from planner.
void CylindersModel::moveAtFirst(int cylid)
{
	if (!d || cylid <= 0 || cylid >= static_cast<int>(d->cylinders.size()))
		return;
	cylinder_t temp_cyl;

	beginMoveRows(QModelIndex(), cylid, cylid, QModelIndex(), 0);
	move_in_range(d->cylinders, cylid, cylid + 1, 0);

	// Create a mapping of cylinder indices:
	// 1) Fill mapping[0]..mapping[cyl] with 0..index
	// 2) Set mapping[cyl] to 0
	// 3) Fill mapping[cyl+1]..mapping[end] with cyl..
	std::vector<int> mapping(d->cylinders.size());
	std::iota(mapping.begin(), mapping.begin() + cylid, 1);
	mapping[cylid] = 0;
	std::iota(mapping.begin() + (cylid + 1), mapping.end(), cylid);
	cylinder_renumber(*d, mapping.data());
	if (inPlanner)
		DivePlannerPointsModel::instance()->cylinderRenumber(mapping.data());
	endMoveRows();
}

// Only invoked from planner.
void CylindersModel::updateDecoDepths(pressure_t olddecopo2)
{
	if (!d || numRows <= 0)
		return;

	pressure_t decopo2;
	decopo2.mbar = prefs.decopo2;
	for (auto &cyl: d->cylinders) {
		/* If the gas's deco MOD matches the old pO2, it will have been automatically calculated and should be updated.
		 * If they don't match, we should leave the user entered depth as it is */
		if (cyl.depth.mm == d->gas_mod(cyl.gasmix, olddecopo2, m_or_ft(3, 10)).mm) {
			cyl.depth = d->gas_mod(cyl.gasmix, decopo2, m_or_ft(3, 10));
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(numRows - 1, COLUMNS - 1));
}

void CylindersModel::updateTrashIcon()
{
	if (!d || numRows <= 0)
		return;

	emit dataChanged(createIndex(0, 0), createIndex(numRows - 1, 0));
}

// Only invoked from planner.
bool CylindersModel::updateBestMixes()
{
	if (!d)
		return false;

	// Check if any of the cylinders are best mixes, update if needed
	bool gasUpdated = false;
	for (auto &cyl: d->cylinders) {
		if (cyl.bestmix_o2) {
			cyl.gasmix.o2 = d->best_o2(d->maxdepth, inPlanner);
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl.gasmix) + get_he(cyl.gasmix) > 1000)
				cyl.gasmix.he.permille = 1000 - get_o2(cyl.gasmix);
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl.depth = d->gas_mod(cyl.gasmix, modpO2, m_or_ft(3, 10));
			gasUpdated = true;
		}
		if (cyl.bestmix_he) {
			cyl.gasmix.he = d->best_he(d->maxdepth, prefs.o2narcotic, cyl.gasmix.o2);
			// fO2 + fHe must not be greater than 1
			if (get_o2(cyl.gasmix) + get_he(cyl.gasmix) > 1000)
				cyl.gasmix.o2.permille = 1000 - get_he(cyl.gasmix);
			gasUpdated = true;
		}
	}
	/* This slot is called when the bottom pO2 and END preferences are updated, we want to
	 * emit dataChanged so MOD and MND are refreshed, even if the gas mix hasn't been changed */
	if (gasUpdated)
		emitDataChanged();
	return gasUpdated;
}

void CylindersModel::emitDataChanged()
{
	emit dataChanged(createIndex(0, 0), createIndex(numRows - 1, COLUMNS - 1));
}

void CylindersModel::cylindersReset(const QVector<dive *> &dives)
{
	// This model only concerns the currently displayed dive. If this is not among the
	// dives that had their cylinders reset, exit.
	if (!d || std::find(dives.begin(), dives.end(), d) == dives.end())
		return;

	// And update the model (the actual change was already performed in the backend)..
	beginResetModel();
	numRows = calcNumRows();
	endResetModel();
}

// Save the cylinder in the given row so that we can revert if the user cancels a type-editing action.
void CylindersModel::initTempCyl(int row)
{
	if (!d || tempRow == row)
		return;
	clearTempCyl();
	const cylinder_t *cyl = d->get_cylinder(row);
	if (!cyl)
		return;

	tempRow = row;
	tempCyl = *cyl;

	dataChanged(index(row, TYPE), index(row, USE));
}

void CylindersModel::clearTempCyl()
{
	if (tempRow < 0)
		return;
	int oldRow = tempRow;
	tempRow = -1;
	tempCyl = cylinder_t();
	dataChanged(index(oldRow, TYPE), index(oldRow, USE));
}

void CylindersModel::commitTempCyl(int row)
{
	if (tempRow < 0)
		return;
	if (row != tempRow)
		return clearTempCyl(); // Huh? We are supposed to commit a different row than the one we stored?
	cylinder_t *cyl = d->get_cylinder(tempRow);
	if (!cyl)
		return;
	// Only submit a command if the type changed
	if (cyl->type.description != tempCyl.type.description || gettextFromC::tr(cyl->type.description.c_str()) != QString::fromStdString(tempCyl.type.description)) {
		if (inPlanner) {
			std::swap(*cyl, tempCyl);
		} else {
			int count = Command::editCylinder(tempRow, tempCyl, Command::EditCylinderType::TYPE, false);
			emit divesEdited(count);
		}
	}
	tempCyl = cylinder_t();
	tempRow = -1;
}
