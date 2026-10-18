// SPDX-License-Identifier: GPL-2.0
#include "qt-models/tankinfomodel.h"
#include "commands/command.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefEquipment.h"
#include "core/string-format.h"

#include <algorithm>

// Format a cylinder size the same way the per-dive cylinder table does:
// working-pressure-rated cuft if the user prefers imperial units and a
// working pressure is known, otherwise plain liters.
static QString get_cylinder_size_string(volume_t size, pressure_t workingpressure)
{
	QString unit;
	int decimals;
	unsigned int ml = size.mliter;
	double value;

	if (workingpressure.mbar && prefs.units.volume == units::CUFT) {
		value = ml_to_cuft(ml) * bar_to_atm(workingpressure.mbar / 1000.0);
		decimals = (value > 20.0) ? 0 : (value > 2.0) ? 1 : 2;
		unit = TankInfoModel::tr("cuft");
	} else {
		value = ml / 1000.0;
		decimals = 1;
		unit = TankInfoModel::tr("L");
	}

	return QString("%L1").arg(value, 0, 'f', decimals) + unit;
}

// Case-insensitive name comparison, used to keep tank_info_table sorted
// alphabetically by name (see sortTankInfoTable() and
// sortedPositionExcluding() below) - matching the Qt::CaseInsensitive
// comparisons already used elsewhere in the codebase for user-visible names
// (e.g. the dive-site search in qt-models/divelocationmodel.cpp).
static bool tank_info_name_less(const std::string &a, const std::string &b)
{
	return QString::compare(QString::fromStdString(a), QString::fromStdString(b), Qt::CaseInsensitive) < 0;
}

// Sort tank_info_table alphabetically by name. There is no manual
// reordering any more (that UI added no real value over a stable,
// predictable automatic order); call this whenever the list is first shown
// or can gain an out-of-place entry (on construction and after add()).
// std::stable_sort, though ties shouldn't occur in practice since names are
// kept unique (see tank_type_name_in_use()).
static void sortTankInfoTable()
{
	std::stable_sort(tank_info_table.begin(), tank_info_table.end(),
			  [](const tank_info &a, const tank_info &b) { return tank_info_name_less(a.name, b.name); });
}

// Where row 'row' would need to move to in order to keep tank_info_table
// sorted, after its own name has already changed (e.g. a rename) but
// nothing else has moved yet. Used instead of a full re-sort so only the
// one row that actually needs to move is touched (see setData()'s
// DESCRIPTION branch).
static int sortedPositionExcluding(int row)
{
	const std::string &name = tank_info_table[row].name;
	int target = 0;
	for (int i = 0; i < (int)tank_info_table.size(); ++i) {
		if (i != row && tank_info_name_less(tank_info_table[i].name, name))
			++target;
	}
	return target;
}

// Checks both the runtime display table and the persisted baseline for an
// existing entry with this name. tank_info_table alone isn't enough:
// reset_tank_info_table() builds it from the baseline only when
// prefs.display_default_tank_infos is set, so with "Show default cylinder
// sizes" off, tank_info_table can omit names that are still present in
// get_tank_info_baseline() - a tank_info_table-only check would let a
// new/renamed name silently collide with one of those baseline-only
// entries, producing two entries with the same name in the persisted
// library, where only the first is ever reachable by name-based lookups
// such as get_tank_info() and remove()'s own baseline mirroring.
static bool tank_type_name_in_use(const std::string &name)
{
	return std::any_of(tank_info_table.begin(), tank_info_table.end(),
			    [&name](const tank_info &ti) { return ti.name == name; }) ||
	       std::any_of(get_tank_info_baseline().begin(), get_tank_info_baseline().end(),
			    [&name](const tank_info &ti) { return ti.name == name; });
}

QVariant TankInfoModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= (int)tank_info_table.size())
		return QVariant();
	if (role == Qt::FontRole)
		return defaultModelFont();
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		const struct tank_info &info = tank_info_table[index.row()];
		auto [size, pressure] = extract_tank_info(info);

		switch (index.column()) {
		case BAR:
			return get_pressure_string(pressure, true);
		case ML:
			return get_cylinder_size_string(size, pressure);
		case DESCRIPTION:
			return QString::fromStdString(info.name);
		}
	}
	return QVariant();
}

Qt::ItemFlags TankInfoModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= (int)tank_info_table.size())
		return Qt::NoItemFlags;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool TankInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;
	if (index.row() < 0 || index.row() >= (int)tank_info_table.size())
		return false;

	struct tank_info &info = tank_info_table[index.row()];
	switch (index.column()) {
	case DESCRIPTION: {
		std::string name = value.toString().trimmed().toStdString();
		if (name.empty() || name == info.name)
			return false;
		// Reject duplicates: a second entry with the same name would just
		// be unreachable, since lookups return the first match. Checks the
		// baseline as well as tank_info_table (see tank_type_name_in_use()),
		// since a baseline-only entry (omitted from tank_info_table when
		// "Show default cylinder sizes" is off) is just as unreachable a
		// collision. This must be checked here, before pushing the command
		// below: tank_info_table is still in its pre-rename state at this
		// point, since nothing has mutated it yet.
		if (tank_type_name_in_use(name))
			return false;
		// Command::RenameCylinderType is the sole owner of a rename (the
		// per-dive cylinder descriptions, tank_info_table, and the
		// persisted baseline - see commands/command_edit.cpp); it performs
		// the actual rename via QUndoStack::push()'s synchronous first
		// redo() call, so info.name already reflects the new value by the
		// time dataChanged() is emitted below. Capture oldName into its own
		// local variable rather than passing info.name directly: the
		// command needs a stable pre-rename value, independent of info.name
		// itself being mutated as a side effect of the call.
		std::string oldName = info.name;
		Command::renameCylinderType(oldName, name);
		// Let callers that track a specific entry by identity (e.g. the
		// "default cylinder" combo in Preferences) follow the rename.
		emit cylinderTypeRenamed(QString::fromStdString(oldName), QString::fromStdString(name));

		// Keep the list sorted: a rename can change which neighbours this
		// entry should now sort next to. Move it directly to its new spot
		// rather than re-sorting the whole table; if it doesn't need to
		// move, fall through to the shared dataChanged()/return below.
		int row = index.row();
		int newRow = sortedPositionExcluding(row);
		if (newRow == row)
			break;
		int dest = newRow > row ? newRow + 1 : newRow;
		if (beginMoveRows(QModelIndex(), row, row, QModelIndex(), dest)) {
			tank_info moved = std::move(tank_info_table[row]);
			tank_info_table.erase(tank_info_table.begin() + row);
			tank_info_table.insert(tank_info_table.begin() + newRow, std::move(moved));
			endMoveRows();
		}
		return true;
	}
	case ML: {
		// The view's default item delegate resubmits the current text
		// whenever an edit is committed, even if nothing was actually
		// typed (e.g. opening the cell and pressing Enter/Tab). Since
		// get_cylinder_size_string() rounds to a couple of decimals,
		// reparsing an unmodified string back through string_to_volume()
		// can silently shift the stored ml value (e.g. a cuft size
		// rounded for display can move by tens of ml on conversion back).
		// Bail out here before that round trip if nothing changed.
		if (value.toString() == data(index, Qt::EditRole).toString())
			return false;
		auto [size, pressure] = extract_tank_info(info);
		volume_t newSize = string_to_volume(qPrintable(value.toString()), pressure);
		if (newSize.mliter <= 0)
			return false;
		update_tank_info_data(info, newSize, pressure);
		// Mirror into the persisted baseline, but only if this type is
		// already a baseline member: a type only shown because the open
		// file uses it must not be auto-promoted into the permanent
		// library just by being resized.
		if (tank_info *baselineInfo = get_tank_info(get_tank_info_baseline(), info.name))
			update_tank_info_data(*baselineInfo, newSize, pressure);
		break;
	}
	case BAR: {
		if (value.toString() == data(index, Qt::EditRole).toString())
			return false;
		auto [size, pressure] = extract_tank_info(info);
		pressure_t newPressure = string_to_pressure(qPrintable(value.toString()));
		if (newPressure.mbar <= 0)
			return false;
		update_tank_info_data(info, size, newPressure);
		if (tank_info *baselineInfo = get_tank_info(get_tank_info_baseline(), info.name))
			update_tank_info_data(*baselineInfo, size, newPressure);
		break;
	}
	default:
		return false;
	}
	emit dataChanged(index, index);
	return true;
}

// Pick a placeholder name guaranteed not to collide with an existing entry
// in either tank_info_table or the baseline (see tank_type_name_in_use()):
// "New type", or "New type 2", "New type 3", ... if that's already taken.
// setData(DESCRIPTION) rejects duplicate names, so add() must not create one
// itself, or the earlier same-named entry would become unreachable by name
// lookups such as get_tank_info() and find_cylinders_with_type_description().
static std::string make_unique_new_type_name()
{
	std::string base = TankInfoModel::tr("New type").toStdString();
	std::string name = base;
	for (int suffix = 2; tank_type_name_in_use(name); ++suffix)
		name = base + " " + std::to_string(suffix);
	return name;
}

int TankInfoModel::add()
{
	std::string name = make_unique_new_type_name();

	// Insert directly at the sorted position instead of appending and then
	// re-sorting: appending would tell the view a row was inserted at the
	// end, then silently move it elsewhere without a proper notification.
	int row = 0;
	while (row < (int)tank_info_table.size() && tank_info_name_less(tank_info_table[row].name, name))
		++row;

	beginInsertRows(QModelIndex(), row, row);
	tank_info_table.insert(tank_info_table.begin() + row, tank_info { name, 0, 0, 0, 0 });
	endInsertRows();
	// A type explicitly created via "Add" is always library-worthy, unlike
	// a type merely displayed because the open file uses it.
	get_tank_info_baseline().push_back(tank_info { std::move(name), 0, 0, 0, 0 });
	return row;
}

void TankInfoModel::remove(int row)
{
	if (row < 0 || row >= (int)tank_info_table.size())
		return;
	std::string name = tank_info_table[row].name;
	// File-only types remain in the runtime catalogue because loaded dives
	// can still reference them. They cannot be removed through the global
	// library editor, as they were never library members to begin with.
	std::vector<tank_info> &baseline = get_tank_info_baseline();
	auto it = std::find_if(baseline.begin(), baseline.end(),
				[&name](const tank_info &ti) { return ti.name == name; });
	if (it == baseline.end())
		return;
	beginRemoveRows(QModelIndex(), row, row);
	tank_info_table.erase(tank_info_table.begin() + row);
	endRemoveRows();
	baseline.erase(it);
	if (qPrefEquipment::default_cylinder() == QString::fromStdString(name))
		qPrefEquipment::set_default_cylinder(QString());
}

int TankInfoModel::rowCount(const QModelIndex&) const
{
	return (int)tank_info_table.size();
}

TankInfoModel::TankInfoModel(QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("Size") << tr("Working pressure"));
	// tank_info_table's order isn't guaranteed alphabetical at this point
	// (e.g. reset_tank_info_table() just concatenates the baseline with
	// whatever cylinder types the open dive log happens to use): sort it
	// once up front so the list is never shown stale or out of order.
	sortTankInfoTable();
}
