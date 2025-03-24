// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

/* equipment.cpp */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include "equipment.h"
#include "gettext.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "errorhelper.h"
#include "pref.h"
#include "range.h"
#include "subsurface-string.h"

cylinder_t::cylinder_t() = default;
cylinder_t::~cylinder_t() = default;

static cylinder_t make_surface_air_cylinder()
{
	cylinder_t res;
	res.cylinder_use = OC_GAS;
	return res;
}
static const cylinder_t surface_air_cylinder = make_surface_air_cylinder();

static void warn_index(size_t i, size_t max)
{
	if (i >= max + 1) {
		report_info("Warning: accessing invalid cylinder %lu (%lu existing)",
			static_cast<unsigned long>(i), static_cast<unsigned long>(max));
	}
}

cylinder_t &cylinder_table::operator[](size_t i)
{
	warn_index(i, size());
	return i < size() ? std::vector<cylinder_t>::operator[](i)
			  : const_cast<cylinder_t &>(surface_air_cylinder);
}

const cylinder_t &cylinder_table::operator[](size_t i) const
{
	warn_index(i, size());
	return i < size() ? std::vector<cylinder_t>::operator[](i)
			  : surface_air_cylinder;
}

weightsystem_t::weightsystem_t() = default;
weightsystem_t::~weightsystem_t() = default;
weightsystem_t::weightsystem_t(weight_t w, std::string desc, bool auto_filled)
	: weight(w), description(std::move(desc)), auto_filled(auto_filled)
{
}

const char *cylinderuse_text[NUM_GAS_USE] = {
	QT_TRANSLATE_NOOP("gettextFromC", "OC-gas"), QT_TRANSLATE_NOOP("gettextFromC", "diluent"), QT_TRANSLATE_NOOP("gettextFromC", "oxygen"), QT_TRANSLATE_NOOP("gettextFromC", "not used")
};

enum cylinderuse cylinderuse_from_text(const char *text)
{
	for (int i = 0; i < static_cast<int>(NUM_GAS_USE); i++) {
		if (same_string(text, cylinderuse_text[i]) || same_string(text, translate("gettextFromC", cylinderuse_text[i])))
			return static_cast<enum cylinderuse>(i);
	}
	return static_cast<enum cylinderuse>(-1);
}

/* Add a metric or an imperial tank info structure. Copies the passed-in string. */
static void add_tank_info_metric(std::vector<tank_info> &table, const std::string &name, int ml, int bar)
{
	table.push_back(tank_info{ name, .ml = ml, .bar = bar });
}

static void add_tank_info_imperial(std::vector<tank_info> &table, const std::string &name, int cuft, int psi)
{
	table.push_back(tank_info{ name, .cuft = cuft, .psi = psi });
}

struct tank_info *get_tank_info(std::vector<tank_info> &table, const std::string &name)
{
	auto it = std::find_if(table.begin(), table.end(), [&name](const tank_info &info)
			       { return info.name == name; });
	return it != table.end() ? &*it : nullptr;
}

void set_tank_info_data(std::vector<tank_info> &table, const std::string &name, volume_t size, pressure_t working_pressure)
{
	struct tank_info *info = get_tank_info(table, name);
	if (info) {
		if (info->ml != 0 || info->bar != 0) {
			info->bar = working_pressure.mbar / 1000;
			info->ml = size.mliter;
		} else {
			info->psi = lrint(to_PSI(working_pressure));
			info->cuft = lrint(ml_to_cuft(size.mliter) * mbar_to_atm(working_pressure.mbar));
		}
	} else {
		// Metric is a better choice as the volume is independent of the working pressure
		add_tank_info_metric(table, name, size.mliter, working_pressure.mbar / 1000);
	}
}

std::pair<volume_t, pressure_t> extract_tank_info(const struct tank_info &info)
{
	pressure_t working_pressure {
		.mbar = static_cast<int32_t>(info.bar != 0 ? info.bar * 1000 : psi_to_mbar(info.psi))
	};
	volume_t size;
	if (info.ml != 0)
		size.mliter = info.ml;
	else if (working_pressure.mbar != 0)
		size.mliter = lrint(cuft_to_l(info.cuft) * 1000 / mbar_to_atm(working_pressure.mbar));
	return std::make_pair(size, working_pressure);
}

std::pair<volume_t, pressure_t> get_tank_info_data(const std::vector<tank_info> &table, const std::string &name)
{
	// Here, we would need a const version of get_tank_info().
	auto it = std::find_if(table.begin(), table.end(), [&name](const tank_info &info)
			       { return info.name == name; });
	return it != table.end() ? extract_tank_info(*it)
				 : std::make_pair(volume_t(), pressure_t());
}

void add_cylinder_description(const cylinder_type_t &type)
{
	const std::string &desc = type.description;
	if (desc.empty())
		return;
	if (std::any_of(tank_info_table.begin(), tank_info_table.end(),
			[&desc](const tank_info &info) { return info.name == desc; }))
		return;
	add_tank_info_metric(tank_info_table, desc, type.size.mliter,
			     type.workingpressure.mbar / 1000);
}

void add_weightsystem_description(const weightsystem_t &weightsystem)
{
	if (weightsystem.description.empty())
		return;

	auto it = std::find_if(ws_info_table.begin(), ws_info_table.end(),
			       [&weightsystem](const ws_info &info)
			       { return info.name == weightsystem.description; });
	if (it != ws_info_table.end()) {
		it->weight = weightsystem.weight;
		return;
	}
	ws_info_table.push_back(ws_info { std::string(weightsystem.description), weightsystem.weight });
}

weight_t get_weightsystem_weight(const std::string &name)
{
	// Also finds translated names (TODO: should only consider non-user items).
	auto it = std::find_if(ws_info_table.begin(), ws_info_table.end(),
			       [&name](const ws_info &info)
			       { return info.name == name || translate("gettextFromC", info.name.c_str()) == name; });
	return it != ws_info_table.end() ? it->weight : weight_t();
}

void cylinder_table::add(int idx, cylinder_t cyl)
{
	insert(begin() + idx, std::move(cyl));
}

bool weightsystem_t::operator==(const weightsystem_t &w2) const
{
	return std::tie(weight.grams, description) ==
	       std::tie(w2.weight.grams, w2.description);
}

volume_t cylinder_t::gas_volume(pressure_t p) const
{
	double bar = p.mbar / 1000.0;
	double z_factor = gas_compressibility_factor(gasmix, bar);
	return volume_t { .mliter = int_cast<int>(type.size.mliter * bar_to_atm(bar) / z_factor) };
}

int find_best_gasmix_match(struct gasmix mix, const struct cylinder_table &cylinders, const enum cylinderuse *use)
{
	int best = -1, score = INT_MAX;

	for (auto [i, match]: enumerated_range(cylinders)) {
		if (use && match.cylinder_use != *use)
			continue;
		int distance = gasmix_distance(mix, match.gasmix);
		if (distance >= score)
			continue;
		best = i;
		score = distance;
	}
	return best;
}

/*
 * We hardcode the most common standard cylinders,
 * we should pick up any other names from the dive
 * logs directly.
 */
static void add_default_tank_infos(std::vector<tank_info> &table)
{
	/* Size-only metric cylinders */
	add_tank_info_metric(table, "10.0L", 10000, 0);
	add_tank_info_metric(table, "11.1L", 11100, 0);

	/* Most common AL cylinders */
	add_tank_info_imperial(table, "AL40", 40, 3000);
	add_tank_info_imperial(table, "AL50", 50, 3000);
	add_tank_info_imperial(table, "AL63", 63, 3000);
	add_tank_info_imperial(table, "AL72", 72, 3000);
	add_tank_info_imperial(table, "AL80", 80, 3000);
	add_tank_info_imperial(table, "AL100", 100, 3300);

	/* Metric AL cylinders */
	add_tank_info_metric(table, "ALU7", 7000, 200);

	/* Somewhat common LP steel cylinders */
	add_tank_info_imperial(table, "LP85", 85, 2640);
	add_tank_info_imperial(table, "LP95", 95, 2640);
	add_tank_info_imperial(table, "LP108", 108, 2640);
	add_tank_info_imperial(table, "LP121", 121, 2640);

	/* Somewhat common HP steel cylinders */
	add_tank_info_imperial(table, "HP65", 65, 3442);
	add_tank_info_imperial(table, "HP80", 80, 3442);
	add_tank_info_imperial(table, "HP100", 100, 3442);
	add_tank_info_imperial(table, "HP117", 117, 3442);
	add_tank_info_imperial(table, "HP119", 119, 3442);
	add_tank_info_imperial(table, "HP130", 130, 3442);

	/* Common European steel cylinders */
	add_tank_info_metric(table, "3L 232 bar", 3000, 232);
	add_tank_info_metric(table, "3L 300 bar", 3000, 300);
	add_tank_info_metric(table, "10L 200 bar", 10000, 200);
	add_tank_info_metric(table, "10L 232 bar", 10000, 232);
	add_tank_info_metric(table, "10L 300 bar", 10000, 300);
	add_tank_info_metric(table, "12L 200 bar", 12000, 200);
	add_tank_info_metric(table, "12L 232 bar", 12000, 232);
	add_tank_info_metric(table, "12L 300 bar", 12000, 300);
	add_tank_info_metric(table, "15L 200 bar", 15000, 200);
	add_tank_info_metric(table, "15L 232 bar", 15000, 232);
	add_tank_info_metric(table, "D7 232 bar", 14000, 232);
	add_tank_info_metric(table, "D7 300 bar", 14000, 300);
	add_tank_info_metric(table, "D8.5 232 bar", 17000, 232);
	add_tank_info_metric(table, "D12 232 bar", 24000, 232);
	add_tank_info_metric(table, "D13 232 bar", 26000, 232);
	add_tank_info_metric(table, "D15 232 bar", 30000, 232);
	add_tank_info_metric(table, "D16 232 bar", 32000, 232);
	add_tank_info_metric(table, "D18 232 bar", 36000, 232);
	add_tank_info_metric(table, "D20 232 bar", 40000, 232);
}

std::vector<tank_info> tank_info_table;
void reset_tank_info_table(std::vector<tank_info> &table)
{
	table.clear();
	if (prefs.display_default_tank_infos)
		add_default_tank_infos(table);

	/* Add cylinders from dive list */
	for (auto &dive: divelog.dives) {
		for (auto &cyl: dive->cylinders)
			add_cylinder_description(cyl.type);
	}
}

/*
 * We hardcode the most common weight system types
 * This is a bit odd as the weight system types don't usually encode weight
 */
struct std::vector<ws_info> ws_info_table = {
	{ QT_TRANSLATE_NOOP("gettextFromC", "integrated"), weight_t() },
	{ QT_TRANSLATE_NOOP("gettextFromC", "belt"), weight_t() },
	{ QT_TRANSLATE_NOOP("gettextFromC", "ankle"), weight_t() },
	{ QT_TRANSLATE_NOOP("gettextFromC", "backplate"), weight_t() },
	{ QT_TRANSLATE_NOOP("gettextFromC", "clip-on"), weight_t() },
};

void remove_cylinder(struct dive *dive, int idx)
{
	dive->cylinders.erase(dive->cylinders.begin() + idx);
}

void weightsystem_table::remove(int idx)
{
	erase(begin() + idx);
}

void weightsystem_table::add(int idx, weightsystem_t ws)
{
	idx = std::clamp(idx, 0, static_cast<int>(size()));
	insert(begin() + idx, std::move(ws));
}

void weightsystem_table::set(int idx, weightsystem_t ws)
{
	if (idx < 0 || static_cast<size_t>(idx) >= size())
		return;
	(*this)[idx] = std::move(ws);
}

/* when planning a dive we need to make sure that all cylinders have a sane depth assigned
 * and if we are tracking gas consumption the pressures need to be reset to start = end = workingpressure */
void reset_cylinders(struct dive *dive, bool track_gas)
{
	pressure_t decopo2 = {.mbar = prefs.decopo2};

	for (cylinder_t &cyl: dive->cylinders) {
		if (cyl.depth.mm == 0) /* if the gas doesn't give a mod, calculate based on prefs */
			cyl.depth = dive->gas_mod(cyl.gasmix, decopo2, m_or_ft(3, 10));
		if (track_gas)
			cyl.start.mbar = cyl.end.mbar = cyl.type.workingpressure.mbar;
		cyl.gas_used = 0_l;
		cyl.deco_gas_used = 0_l;
	}
}

static void copy_cylinder_type(const cylinder_t &s, cylinder_t &d)
{
	d.type = s.type;
	d.gasmix = s.gasmix;
	d.depth = s.depth;
	d.cylinder_use = s.cylinder_use;
	d.manually_added = true;
}

/* copy the equipment data part of the cylinders but keep pressures */
void copy_cylinder_types(const struct dive *s, struct dive *d)
{
	if (!s || !d)
		return;

	for (size_t i = 0; i < s->cylinders.size() && i < d->cylinders.size(); i++)
		copy_cylinder_type(s->cylinders[i], d->cylinders[i]);

	for (size_t i = d->cylinders.size(); i < s->cylinders.size(); i++)
		d->cylinders.push_back(s->cylinders[i]);
}

/* if a default cylinder is set, use that */
void fill_default_cylinder(const struct dive *dive, cylinder_t *cyl)
{
	const std::string &cyl_name = prefs.default_cylinder;
	pressure_t pO2 = {.mbar = int_cast<int>(prefs.modpO2 * 1000.0)};

	if (cyl_name.empty())
		return;
	for (auto &ti: tank_info_table) {
		if (ti.name == cyl_name) {
			cyl->type.description = ti.name;
			if (ti.ml) {
				cyl->type.size.mliter = ti.ml;
				cyl->type.workingpressure.mbar = ti.bar * 1000;
			} else {
				cyl->type.workingpressure.mbar = psi_to_mbar(ti.psi);
				if (ti.psi)
					cyl->type.size.mliter = lrint(cuft_to_l(ti.cuft) * 1000 / bar_to_atm(psi_to_bar(ti.psi)));
			}
			// MOD of air
			cyl->depth = dive->gas_mod(cyl->gasmix, pO2, 1_mm);
			return;
		}
	}
}

cylinder_t default_cylinder(const struct dive *d)
{
	cylinder_t res;
	fill_default_cylinder(d, &res);
	return res;
}

cylinder_t create_new_cylinder(const struct dive *d)
{
	cylinder_t cyl = default_cylinder(d);
	cyl.start = cyl.type.workingpressure;
	cyl.cylinder_use = OC_GAS;
	return cyl;
}

cylinder_t create_new_manual_cylinder(const struct dive *d)
{
	cylinder_t cyl = create_new_cylinder(d);
	cyl.manually_added = true;
	return cyl;
}

void add_default_cylinder(struct dive *d)
{
	// Only add if there are no cylinders yet
	if (!d->cylinders.empty())
		return;

	cylinder_t cyl;
	if (!prefs.default_cylinder.empty()) {
		cyl = create_new_cylinder(d);
	} else {
		// roughly an AL80
		cyl.type.description = translate("gettextFromC", "unknown");
		cyl.type.size = 11100_ml;
		cyl.type.workingpressure = 207_bar;
	}
	d->cylinders.add(0, std::move(cyl));
	reset_cylinders(d, false);
}

static bool show_cylinder(const struct dive *d, int i)
{
	if (d->is_cylinder_used(i))
		return true;

	const cylinder_t &cyl = d->cylinders[i];
	if (cyl.start.mbar || cyl.sample_start.mbar ||
	    cyl.end.mbar || cyl.sample_end.mbar)
		return true;
	if (cyl.manually_added)
		return true;

	/*
	 * The cylinder has some data, but none of it is very interesting,
	 * it has no pressures and no gas switches. Do we want to show it?
	 */
	return false;
}

/* The unused cylinders at the end of the cylinder list are hidden. */
int first_hidden_cylinder(const struct dive *d)
{
	size_t res = d->cylinders.size();
	while (res > 0 && !show_cylinder(d, res - 1))
		--res;
	return static_cast<int>(res);
}

#ifdef DEBUG_CYL
void dump_cylinders(struct dive *dive, bool verbose)
{
	printf("Cylinder list:\n");
	for (int i = 0; i < dive->cylinders; i++) {
		cylinder_t *cyl = dive->get_cylinder(i);

		printf("%02d: Type     %s, %3.1fl, %3.0fbar\n", i, cyl->type.description.c_str(), cyl->type.size.mliter / 1000.0, cyl->type.workingpressure.mbar / 1000.0);
		printf("    Gasmix   O2 %2.0f%% He %2.0f%%\n", cyl->gasmix.o2.permille / 10.0, cyl->gasmix.he.permille / 10.0);
		printf("    Pressure Start %3.0fbar End %3.0fbar Sample start %3.0fbar Sample end %3.0fbar\n", cyl->start.mbar / 1000.0, cyl->end.mbar / 1000.0, cyl->sample_start.mbar / 1000.0, cyl->sample_end.mbar / 1000.0);
		if (verbose) {
			printf("    Depth    %3.0fm\n", cyl->depth.mm / 1000.0);
			printf("    Added    %s\n", (cyl->manually_added ? "manually" : ""));
			printf("    Gas used Bottom %5.0fl Deco %5.0fl\n", cyl->gas_used.mliter / 1000.0, cyl->deco_gas_used.mliter / 1000.0);
			printf("    Use      %d\n", cyl->cylinder_use);
			printf("    Bestmix  %s %s\n", (cyl->bestmix_o2 ? "O2" : "  "), (cyl->bestmix_he ? "He" : "  "));
		}
	}
}
#endif
