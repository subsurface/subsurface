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
#include "subsurface-string.h"
#include "table.h"

/* Warning: this has strange semantics for C-code! Not the weightsystem object
 * is freed, but the data it references. The object itself is passed in by value.
 * This is due to the fact how the table macros work.
 */
void free_weightsystem(weightsystem_t ws)
{
	free((void *)ws.description);
	ws.description = NULL;
}

void free_cylinder(cylinder_t c)
{
	free((void *)c.type.description);
	c.type.description = NULL;
}

void copy_weights(const struct weightsystem_table *s, struct weightsystem_table *d)
{
	clear_weightsystem_table(d);
	for (int i = 0; i < s->nr; i++)
		add_cloned_weightsystem(d, s->weightsystems[i]);
}

void copy_cylinders(const struct cylinder_table *s, struct cylinder_table *d)
{
	int i;
	clear_cylinder_table(d);
	for (i = 0; i < s->nr; i++)
		add_cloned_cylinder(d, s->cylinders[i]);
}

/* weightsystem table functions */
//static MAKE_GET_IDX(weightsystem_table, weightsystem_t, weightsystems)
static MAKE_GROW_TABLE(weightsystem_table, weightsystem_t, weightsystems)
//static MAKE_GET_INSERTION_INDEX(weightsystem_table, weightsystem_t, weightsystems, weightsystem_less_than)
MAKE_ADD_TO(weightsystem_table, weightsystem_t, weightsystems)
static MAKE_REMOVE_FROM(weightsystem_table, weightsystems)
//MAKE_SORT(weightsystem_table, weightsystem_t, weightsystems, comp_weightsystems)
//MAKE_REMOVE(weightsystem_table, weightsystem_t, weightsystem)
MAKE_CLEAR_TABLE(weightsystem_table, weightsystems, weightsystem)

/* cylinder table functions */
//static MAKE_GET_IDX(cylinder_table, cylinder_t, cylinders)
static MAKE_GROW_TABLE(cylinder_table, cylinder_t, cylinders)
//static MAKE_GET_INSERTION_INDEX(cylinder_table, cylinder_t, cylinders, cylinder_less_than)
static MAKE_ADD_TO(cylinder_table, cylinder_t, cylinders)
static MAKE_REMOVE_FROM(cylinder_table, cylinders)
//MAKE_SORT(cylinder_table, cylinder_t, cylinders, comp_cylinders)
//MAKE_REMOVE(cylinder_table, cylinder_t, cylinder)
MAKE_CLEAR_TABLE(cylinder_table, cylinders, cylinder)

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
		static_cast<int32_t>(info.bar != 0 ? info.bar * 1000 : psi_to_mbar(info.psi))
	};
	volume_t size { 0 };
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
				 : std::make_pair(volume_t{0}, pressure_t{0});
}

void add_cylinder_description(const cylinder_type_t &type)
{
	std::string desc = type.description ? type.description : std::string();
	if (desc.empty())
		return;
	if (std::any_of(tank_info_table.begin(), tank_info_table.end(),
			[&type](const tank_info &info) { return info.name == type.description; }))
		return;
	add_tank_info_metric(tank_info_table, desc, type.size.mliter,
			     type.workingpressure.mbar / 1000);
}

void add_weightsystem_description(const weightsystem_t &weightsystem)
{
	if (empty_string(weightsystem.description))
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

weightsystem_t clone_weightsystem(weightsystem_t ws)
{
	weightsystem_t res = { ws.weight, copy_string(ws.description), ws.auto_filled };
	return res;
}

/* Add a clone of a weightsystem to the end of a weightsystem table.
 * Cloned means that the description-string is copied. */
void add_cloned_weightsystem(struct weightsystem_table *t, weightsystem_t ws)
{
	add_to_weightsystem_table(t, t->nr, clone_weightsystem(ws));
}

cylinder_t clone_cylinder(cylinder_t cyl)
{
	cylinder_t res = cyl;
	res.type.description = copy_string(res.type.description);
	return res;
}

void add_cylinder(struct cylinder_table *t, int idx, cylinder_t cyl)
{
	add_to_cylinder_table(t, idx, cyl);
	/* FIXME: This is a horrible hack: we make sure that at the end of
	 * every single cylinder table there is an empty cylinder that can
	 * be used by the planner as "surface air" cylinder. Fix this.
	 */
	add_to_cylinder_table(t, t->nr, empty_cylinder);
	t->nr--;
	t->cylinders[t->nr].cylinder_use = NOT_USED;
}

/* Add a clone of a cylinder to the end of a cylinder table.
 * Cloned means that the description-string is copied. */
void add_cloned_cylinder(struct cylinder_table *t, cylinder_t cyl)
{
	add_cylinder(t, t->nr, clone_cylinder(cyl));
}

bool same_weightsystem(weightsystem_t w1, weightsystem_t w2)
{
	return w1.weight.grams == w2.weight.grams &&
	       same_string(w1.description, w2.description);
}

void get_gas_string(struct gasmix gasmix, char *text, int len)
{
	if (gasmix_is_air(gasmix))
		snprintf(text, len, "%s", translate("gettextFromC", "air"));
	else if (get_he(gasmix) == 0 && get_o2(gasmix) < 1000)
		snprintf(text, len, translate("gettextFromC", "EAN%d"), (get_o2(gasmix) + 5) / 10);
	else if (get_he(gasmix) == 0 && get_o2(gasmix) == 1000)
		snprintf(text, len, "%s", translate("gettextFromC", "oxygen"));
	else
		snprintf(text, len, "(%d/%d)", (get_o2(gasmix) + 5) / 10, (get_he(gasmix) + 5) / 10);
}

/* Returns a static char buffer - only good for immediate use by printf etc */
const char *gasname(struct gasmix gasmix)
{
	static char gas[64];
	get_gas_string(gasmix, gas, sizeof(gas));
	return gas;
}

int gas_volume(const cylinder_t *cyl, pressure_t p)
{
	double bar = p.mbar / 1000.0;
	double z_factor = gas_compressibility_factor(cyl->gasmix, bar);
	return lrint(cyl->type.size.mliter * bar_to_atm(bar) / z_factor);
}

int find_best_gasmix_match(struct gasmix mix, const struct cylinder_table *cylinders)
{
	int i;
	int best = -1, score = INT_MAX;

	for (i = 0; i < cylinders->nr; i++) {
		const cylinder_t *match;
		int distance;

		match = cylinders->cylinders + i;
		distance = gasmix_distance(mix, match->gasmix);
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
	add_tank_info_metric(table, "10.0ℓ", 10000, 0);
	add_tank_info_metric(table, "11.1ℓ", 11100, 0);

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
	add_tank_info_metric(table, "3ℓ 232 bar", 3000, 232);
	add_tank_info_metric(table, "3ℓ 300 bar", 3000, 300);
	add_tank_info_metric(table, "10ℓ 200 bar", 10000, 200);
	add_tank_info_metric(table, "10ℓ 232 bar", 10000, 232);
	add_tank_info_metric(table, "10ℓ 300 bar", 10000, 300);
	add_tank_info_metric(table, "12ℓ 200 bar", 12000, 200);
	add_tank_info_metric(table, "12ℓ 232 bar", 12000, 232);
	add_tank_info_metric(table, "12ℓ 300 bar", 12000, 300);
	add_tank_info_metric(table, "15ℓ 200 bar", 15000, 200);
	add_tank_info_metric(table, "15ℓ 232 bar", 15000, 232);
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
	for (int i = 0; i < divelog.dives->nr; ++i) {
		const struct dive *dive = divelog.dives->dives[i];
		for (int j = 0; j < dive->cylinders.nr; j++) {
			const cylinder_t *cyl = get_cylinder(dive, j);
			add_cylinder_description(cyl->type);
		}
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
	remove_from_cylinder_table(&dive->cylinders, idx);
}

void remove_weightsystem(struct dive *dive, int idx)
{
	remove_from_weightsystem_table(&dive->weightsystems, idx);
}

// ws is cloned.
void set_weightsystem(struct dive *dive, int idx, weightsystem_t ws)
{
	if (idx < 0 || idx >= dive->weightsystems.nr)
		return;
	free_weightsystem(dive->weightsystems.weightsystems[idx]);
	dive->weightsystems.weightsystems[idx] = clone_weightsystem(ws);
}

/* when planning a dive we need to make sure that all cylinders have a sane depth assigned
 * and if we are tracking gas consumption the pressures need to be reset to start = end = workingpressure */
void reset_cylinders(struct dive *dive, bool track_gas)
{
	pressure_t decopo2 = {.mbar = prefs.decopo2};

	for (int i = 0; i < dive->cylinders.nr; i++) {
		cylinder_t *cyl = get_cylinder(dive, i);
		if (cyl->depth.mm == 0) /* if the gas doesn't give a mod, calculate based on prefs */
			cyl->depth = gas_mod(cyl->gasmix, decopo2, dive, M_OR_FT(3,10));
		if (track_gas)
			cyl->start.mbar = cyl->end.mbar = cyl->type.workingpressure.mbar;
		cyl->gas_used.mliter = 0;
		cyl->deco_gas_used.mliter = 0;
	}
}

static void copy_cylinder_type(const cylinder_t *s, cylinder_t *d)
{
	free_cylinder(*d);
	d->type = s->type;
	d->type.description = s->type.description ? strdup(s->type.description) : NULL;
	d->gasmix = s->gasmix;
	d->depth = s->depth;
	d->cylinder_use = s->cylinder_use;
	d->manually_added = true;
}

/* copy the equipment data part of the cylinders but keep pressures */
void copy_cylinder_types(const struct dive *s, struct dive *d)
{
	int i;
	if (!s || !d)
		return;

	for (i = 0; i < s->cylinders.nr && i < d->cylinders.nr; i++)
		copy_cylinder_type(get_cylinder(s, i), get_cylinder(d, i));

	for ( ; i < s->cylinders.nr; i++)
		add_cloned_cylinder(&d->cylinders, *get_cylinder(s, i));
}

cylinder_t *add_empty_cylinder(struct cylinder_table *t)
{
	cylinder_t cyl = empty_cylinder;
	cyl.type.description = strdup("");
	add_cylinder(t, t->nr, cyl);
	return &t->cylinders[t->nr - 1];
}

/* access to cylinders is controlled by two functions:
 * - get_cylinder() returns the cylinder of a dive and supposes that
 *   the cylinder with the given index exists. If it doesn't, an error
 *   message is printed and NULL returned.
 * - get_or_create_cylinder() creates an empty cylinder if it doesn't exist.
 *   Multiple cylinders might be created if the index is bigger than the
 *   number of existing cylinders
 */
cylinder_t *get_cylinder(const struct dive *d, int idx)
{
	/* FIXME: The planner uses a dummy cylinder one past the official number of cylinders
	 * in the table to mark no-cylinder surface interavals. This is horrendous. Fix ASAP. */
	// if (idx < 0 || idx >= d->cylinders.nr) {
	if (idx < 0 || idx >= d->cylinders.nr + 1 || idx >= d->cylinders.allocated) {
		report_info("Warning: accessing invalid cylinder %d (%d existing)", idx, d->cylinders.nr);
		return NULL;
	}
	return &d->cylinders.cylinders[idx];
}

cylinder_t *get_or_create_cylinder(struct dive *d, int idx)
{
	if (idx < 0) {
		report_info("Warning: accessing invalid cylinder %d", idx);
		return NULL;
	}
	while (idx >= d->cylinders.nr)
		add_empty_cylinder(&d->cylinders);
	return &d->cylinders.cylinders[idx];
}

/* if a default cylinder is set, use that */
void fill_default_cylinder(const struct dive *dive, cylinder_t *cyl)
{
	const char *cyl_name = prefs.default_cylinder;
	pressure_t pO2 = {.mbar = static_cast<int>(lrint(prefs.modpO2 * 1000.0))};

	if (!cyl_name)
		return;
	for (auto &ti: tank_info_table) {
		if (ti.name == cyl_name) {
			cyl->type.description = strdup(ti.name.c_str());
			if (ti.ml) {
				cyl->type.size.mliter = ti.ml;
				cyl->type.workingpressure.mbar = ti.bar * 1000;
			} else {
				cyl->type.workingpressure.mbar = psi_to_mbar(ti.psi);
				if (ti.psi)
					cyl->type.size.mliter = lrint(cuft_to_l(ti.cuft) * 1000 / bar_to_atm(psi_to_bar(ti.psi)));
			}
			// MOD of air
			cyl->depth = gas_mod(cyl->gasmix, pO2, dive, 1);
			return;
		}
	}
}

cylinder_t create_new_cylinder(const struct dive *d)
{
	cylinder_t cyl = empty_cylinder;
	fill_default_cylinder(d, &cyl);
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
	if (d->cylinders.nr > 0)
		return;

	cylinder_t cyl;
	if (!empty_string(prefs.default_cylinder)) {
		cyl = create_new_cylinder(d);
	} else {
		cyl = empty_cylinder;
		// roughly an AL80
		cyl.type.description = strdup(translate("gettextFromC", "unknown"));
		cyl.type.size.mliter = 11100;
		cyl.type.workingpressure.mbar = 207000;
	}
	add_cylinder(&d->cylinders, 0, cyl);
	reset_cylinders(d, false);
}

static bool show_cylinder(const struct dive *d, int i)
{
	if (is_cylinder_used(d, i))
		return true;

	const cylinder_t *cyl = &d->cylinders.cylinders[i];
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

/* The unused cylinders at the end of the cylinder list are hidden. */
int first_hidden_cylinder(const struct dive *d)
{
	int res = d->cylinders.nr;
	while (res > 0 && !show_cylinder(d, res - 1))
		--res;
	return res;
}

#ifdef DEBUG_CYL
void dump_cylinders(struct dive *dive, bool verbose)
{
	printf("Cylinder list:\n");
	for (int i = 0; i < dive->cylinders; i++) {
		cylinder_t *cyl = get_cylinder(dive, i);

		printf("%02d: Type     %s, %3.1fl, %3.0fbar\n", i, cyl->type.description, cyl->type.size.mliter / 1000.0, cyl->type.workingpressure.mbar / 1000.0);
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
