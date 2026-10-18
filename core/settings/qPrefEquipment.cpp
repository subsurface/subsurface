// SPDX-License-Identifier: GPL-2.0
#include "qPrefEquipment.h"
#include "qPrefPrivate.h"
#include "core/equipment.h"

#include <QSettings>

static const QString group = QStringLiteral("Equipment");

qPrefEquipment *qPrefEquipment::instance()
{
	static qPrefEquipment *self = new qPrefEquipment;
	return self;
}

void qPrefEquipment::loadSync(bool doSync)
{
	disk_default_cylinder(doSync);
	disk_include_unused_tanks(doSync);
	disk_display_default_tank_infos(doSync);
	if (!doSync)
		load_tank_info_table_baseline();
}

HANDLE_PREFERENCE_TXT(Equipment, "default_cylinder", default_cylinder);
// Keeping the persisted preference name the same to avoid resetting this for everybody
HANDLE_PREFERENCE_BOOL(Equipment, "display_unused_tanks", include_unused_tanks);
HANDLE_PREFERENCE_BOOL(Equipment, "display_default_tank_infos", display_default_tank_infos);

// ---- Persistence of the global cylinder-type list (tank_info_table) ----
//
// Stored as "Equipment/TankTypes/count" plus one "Name"/"Cuft"/"Ml"/"Psi"/
// "Bar" entry per index, mirroring the plain indexed-key style already used
// for MainWindow's "Recent_Files" QSettings group, since tank_info is a
// variable-length list rather than a single scalar value.

static QString tankTypeKey(const QString &field, int index)
{
	return group + QStringLiteral("/TankTypes/") + field + QString::number(index);
}

// save_tank_info_table() never writes more entries than tank_info_table can
// plausibly hold in practice; a persisted count outside this range cannot be
// its output and indicates a corrupted or hand-edited settings file.
static const int max_saved_tank_info_count = 10000;

// Returns true (and fills 'table') if the user has ever saved customisations
// via the "Manage cylinder types" editor, even if they deleted every entry;
// returns false if nothing has been persisted yet, in which case the
// hard-coded defaults should be used instead.
static bool load_saved_tank_info_table(std::vector<tank_info> &table)
{
	QSettings s;
	QString countKey = group + QStringLiteral("/TankTypes/count");
	if (!s.contains(countKey))
		return false;

	bool ok = false;
	int count = s.value(countKey).toInt(&ok);
	// A negative count would underflow to a huge size_t in reserve() below;
	// an implausibly large one risks an oversized allocation outright. Both
	// are signs of corrupted data, so fall back to the hard-coded defaults
	// exactly as if nothing had been persisted yet.
	if (!ok || count < 0 || count > max_saved_tank_info_count)
		return false;

	table.clear();
	table.reserve(count);
	for (int i = 0; i < count; ++i) {
		QString name = s.value(tankTypeKey(QStringLiteral("Name"), i)).toString();
		if (name.isEmpty())
			continue;
		table.push_back(tank_info {
			name.toStdString(),
			s.value(tankTypeKey(QStringLiteral("Cuft"), i), 0).toInt(),
			s.value(tankTypeKey(QStringLiteral("Ml"), i), 0).toInt(),
			s.value(tankTypeKey(QStringLiteral("Psi"), i), 0).toInt(),
			s.value(tankTypeKey(QStringLiteral("Bar"), i), 0).toInt(),
		});
	}
	return true;
}

void qPrefEquipment::load_tank_info_table_baseline()
{
	std::vector<tank_info> table;
	if (!load_saved_tank_info_table(table))
		table = default_tank_info_table();
	set_tank_info_baseline(std::move(table));
}

// Declared in core/equipment.h, defined here because this is the one place
// in core/ that already owns QSettings access for the Equipment group.
//
// Persists get_tank_info_baseline(), NOT tank_info_table: the baseline is
// the permanent, user-curated library, already kept up to date in place by
// every "Manage cylinder types" editor action (see qt-models/tankinfomodel.cpp
// and Command::RenameCylinderType in commands/command_edit.cpp), whereas
// tank_info_table is just the current session's runtime display/picker view
// (baseline plus whatever the open file happens to use - see
// reset_tank_info_table()). Persisting tank_info_table instead would
// permanently promote every cylinder type the open file uses into the
// library, and would silently drop baseline members the open file doesn't
// happen to reference.
void save_tank_info_table()
{
	const std::vector<tank_info> &baseline = get_tank_info_baseline();

	QSettings s;
	s.remove(group + QStringLiteral("/TankTypes"));
	s.setValue(group + QStringLiteral("/TankTypes/count"), static_cast<int>(baseline.size()));
	for (int i = 0; i < static_cast<int>(baseline.size()); ++i) {
		const tank_info &ti = baseline[i];
		s.setValue(tankTypeKey(QStringLiteral("Name"), i), QString::fromStdString(ti.name));
		s.setValue(tankTypeKey(QStringLiteral("Cuft"), i), ti.cuft);
		s.setValue(tankTypeKey(QStringLiteral("Ml"), i), ti.ml);
		s.setValue(tankTypeKey(QStringLiteral("Psi"), i), ti.psi);
		s.setValue(tankTypeKey(QStringLiteral("Bar"), i), ti.bar);
	}
}
