// SPDX-License-Identifier: GPL-2.0
#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "gas.h"

#include <string>
#include <vector>

struct dive;

enum cylinderuse {OC_GAS, DILUENT, OXYGEN, NOT_USED, TRAVEL_OC, NUM_GAS_USE}; // The different uses for cylinders
extern const char *cylinderuse_text[NUM_GAS_USE];

struct cylinder_type_t
{
	volume_t size;
	pressure_t workingpressure;
	std::string description; /* "LP85", "AL72", "AL80", "HP100+" or whatever */
};

struct cylinder_t
{
	cylinder_type_t type;
	struct gasmix gasmix = gasmix_air;
	pressure_t start, end, sample_start, sample_end;
	depth_t depth;
	bool manually_added = false;
	/* Do not replace an explicitly reported start pressure with a sample. */
	bool start_pressure_is_explicit = false;
	volume_t gas_used;
	volume_t deco_gas_used;
	enum cylinderuse cylinder_use = OC_GAS;
	bool bestmix_o2 = false;
	bool bestmix_he = false;

	cylinder_t();
	~cylinder_t();
	cylinder_t(const cylinder_t &) = default;
	cylinder_t(cylinder_t &&) = default;
	cylinder_t &operator=(const cylinder_t &) = default;
	cylinder_t &operator=(cylinder_t &&) = default;

	volume_t gas_volume(pressure_t p) const; /* Volume of a cylinder at pressure 'p' */
};

// True for a cylinder breathed open-circuit, whether or not it is still
// available for the rest of the dive (OC_GAS or TRAVEL_OC).
extern bool is_oc(const struct cylinder_t &cyl);

/* Table of cylinders.
 * This is a crazy class: it is basically a std::vector<>, but overrides
 * the [] accessor functions and allows out-of-bound accesses.
 * This is used in the planner, which uses "max_index + 1" for the
 * surface air cylinder.
 * Note: an out-of-bound access returns a reference to an object with
 * static linkage that MUST NOT be written into.
 * Yes, this is all very mad, but it grew historically.
 */
struct cylinder_table : public std::vector<cylinder_t> {
	cylinder_t &operator[](size_t i);
	const cylinder_t &operator[](size_t i) const;

	void add(int idx, cylinder_t cyl);
};

struct weightsystem_t
{
	weight_t weight;
	std::string description; /* "integrated", "belt", "ankle" */
	bool auto_filled = false; /* weight was automatically derived from the type */

	weightsystem_t();
	weightsystem_t(weight_t w, std::string desc, bool auto_filled);
	~weightsystem_t();
	weightsystem_t(const weightsystem_t &) = default;
	weightsystem_t(weightsystem_t &&) = default;
	weightsystem_t &operator=(const weightsystem_t &) = default;
	weightsystem_t &operator=(weightsystem_t &&) = default;

	bool operator==(const weightsystem_t &w2) const;
};

/* Table of weightsystems. Attention: this stores weightsystems,
 * *not* pointers * to weightsystems. Therefore pointers to
 * weightsystems are *not* stable.
 */
struct weightsystem_table : public std::vector<weightsystem_t> {
	void add(int idx, weightsystem_t ws);
	void set(int idx, weightsystem_t ws);
	void remove(int idx);
};

extern enum cylinderuse cylinderuse_from_text(const char *text);
extern void copy_cylinder_types(const struct dive *s, struct dive *d);
extern void remove_cylinder(struct dive *dive, int idx);
extern depth_t calculate_deco_switch_depth(const struct dive *dive, struct gasmix gasmix);
extern void normalize_imported_cylinder_depths(struct dive *dive);
extern void reset_cylinders(struct dive *dive, bool track_gas);
extern int find_best_gasmix_match(struct gasmix mix, const struct cylinder_table &cylinders, const enum cylinderuse *use);
extern void fill_default_cylinder(const struct dive *dive, cylinder_t *cyl); /* dive is needed to fill out MOD, which depends on salinity. */
extern cylinder_t default_cylinder(const struct dive *d);
extern cylinder_t create_new_manual_cylinder(const struct dive *dive); /* dive is needed to fill out MOD, which depends on salinity. */
extern void add_default_cylinder(struct dive *dive);
extern int first_hidden_cylinder(const struct dive *d);
extern std::vector<int> get_cylinder_map_for_remove(int count, int n);
extern std::vector<int> get_cylinder_map_for_add(int count, int n);
#ifdef DEBUG_CYL
extern void dump_cylinders(struct dive *dive, bool verbose);
#endif

struct ws_info {
	std::string name;
	weight_t weight;
};
extern std::vector<ws_info> ws_info_table;
extern weight_t get_weightsystem_weight(const std::string &name); // returns 0 if not found
extern void add_weightsystem_description(const weightsystem_t &);

struct tank_info {
	std::string name;
	int cuft, ml, psi, bar;
};

extern std::vector<tank_info> tank_info_table;
extern struct tank_info *get_tank_info(std::vector<tank_info> &table, const std::string &name);
extern void update_tank_info_data(struct tank_info &info, volume_t size, pressure_t working_pressure);
extern void set_tank_info_data(std::vector<tank_info> &table, const std::string &name, volume_t size, pressure_t working_pressure);
extern std::pair<volume_t, pressure_t> extract_tank_info(const struct tank_info &info);
extern std::pair<volume_t, pressure_t> get_tank_info_data(const std::vector<tank_info> &table, const std::string &name);
extern void add_cylinder_description(const cylinder_type_t &);
extern void reset_tank_info_table(std::vector<tank_info> &table);

// Find every cylinder in the currently loaded dive log (divelog.dives) whose
// type description exactly matches name, returning each as a (dive,
// cylinder-index) pair. Used by Command::RenameCylinderType
// (commands/command_edit.cpp) - the sole owner of a global cylinder-type
// rename made through the "Manage cylinder types" editor - to cascade the
// rename into the dives that still reference the old name, alongside the
// matching tank_info_table and get_tank_info_baseline() entries. Otherwise
// the old name would linger in per-dive data and reappear as a spurious
// duplicate the next time reset_tank_info_table() merges it back into
// tank_info_table (e.g. on reopening Preferences or reloading the file).
extern std::vector<std::pair<struct dive *, int>> find_cylinders_with_type_description(const std::string &name);

// Install the list of cylinder types used as the persisted "baseline" (see
// tank_info_baseline in core/equipment.cpp): the entries shown before any
// per-dive-log-specific types are layered on top by reset_tank_info_table(),
// and the only list save_tank_info_table() ever writes to QSettings. Until
// this is called, the baseline is the hard-coded list of common cylinders
// built by add_default_tank_infos(). core/settings/qPrefEquipment.cpp calls
// this once at startup to install the user's persisted customisations
// instead, if any have been saved (and, if none have, explicitly
// re-installs the hard-coded list below, so that a later reload - e.g. the
// preferences dialog's own "reload everything" path - cannot leave a
// previously installed baseline stuck in place). Wholesale replacement like
// this is for startup/reload only; the "Manage cylinder types" editor's
// individual add/resize/remove/rename actions instead mutate the baseline
// in place via get_tank_info_baseline() below, with skip-if-absent
// semantics for anything that isn't already a baseline member.
extern void set_tank_info_baseline(std::vector<tank_info> table);

// Mutable access to the persisted cylinder-type baseline (lazily seeded
// from add_default_tank_infos() on first use, same as before
// set_tank_info_baseline() is ever called). Used by reset_tank_info_table()
// below to build the runtime tank_info_table, and by the "Manage cylinder
// types" editor (qt-models/tankinfomodel.cpp) and Command::RenameCylinderType
// (commands/command_edit.cpp) to mirror individual add/resize/remove/rename
// edits into the list that save_tank_info_table() actually persists, without
// ever auto-promoting a type that is only shown because the open file uses
// it (skip-if-absent).
extern std::vector<tank_info> &get_tank_info_baseline();

// A fresh copy of the hard-coded list of common cylinder types (the same
// list used to seed the baseline the very first time, before any
// persisted customisation has ever been installed).
extern std::vector<tank_info> default_tank_info_table();

// Persist the current contents of the cylinder-type baseline (see
// get_tank_info_baseline() above - NOT tank_info_table, which is just the
// current session's runtime display/picker view) so that renames, resizes,
// additions and removals made through the "Manage cylinder types" editor
// survive application restarts. The baseline is already mutated in place by
// the editor as each edit happens (see get_tank_info_baseline() above), so
// this only needs to serialize it; there is nothing left to install as a
// new baseline. Implemented in core/settings/qPrefEquipment.cpp, which owns
// QSettings access for the Equipment preferences group. Deliberately not
// tied to the generic qPref::sync() (called at shutdown by every Subsurface
// binary, including the non-interactive CLI tools): those populate
// tank_info_table from whatever file they happen to process and must never
// overwrite the user's persisted cylinder-type list as a side effect of a
// batch operation.
extern void save_tank_info_table();

#endif // EQUIPMENT_H
