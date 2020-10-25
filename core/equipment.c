// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

/* equipment.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>
#include "equipment.h"
#include "gettext.h"
#include "dive.h"
#include "display.h"
#include "divelist.h"
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

int cylinderuse_from_text(const char *text)
{
	for (enum cylinderuse i = 0; i < NUM_GAS_USE; i++) {
		if (same_string(text, cylinderuse_text[i]) || same_string(text, translate("gettextFromC", cylinderuse_text[i])))
			return i;
	}
	return -1;
}

/* placeholders for a few functions that we need to redesign for the Qt UI */
void add_cylinder_description(const cylinder_type_t *type)
{
	const char *desc;
	int i;

	desc = type->description;
	if (!desc)
		return;
	for (i = 0; i < MAX_TANK_INFO && tank_info[i].name != NULL; i++) {
		if (strcmp(tank_info[i].name, desc) == 0)
			return;
	}
	if (i < MAX_TANK_INFO) {
		// FIXME: leaked on exit
		tank_info[i].name = strdup(desc);
		tank_info[i].ml = type->size.mliter;
		tank_info[i].bar = type->workingpressure.mbar / 1000;
	}
}

void add_weightsystem_description(const weightsystem_t *weightsystem)
{
	const char *desc;
	int i;

	desc = weightsystem->description;
	if (!desc)
		return;
	for (i = 0; i < MAX_WS_INFO && ws_info[i].name != NULL; i++) {
		if (strcmp(ws_info[i].name, desc) == 0) {
			ws_info[i].grams = weightsystem->weight.grams;
			return;
		}
	}
	if (i < MAX_WS_INFO) {
		// FIXME: leaked on exit
		ws_info[i].name = strdup(desc);
		ws_info[i].grams = weightsystem->weight.grams;
	}
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

/* Add a clone of a weightsystem to the end of a weightsystem table.
 * Cloned means that the description-string is copied. */
void add_cloned_weightsystem_at(struct weightsystem_table *t, weightsystem_t ws)
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
struct tank_info_t tank_info[MAX_TANK_INFO] = {
	/* Need an empty entry for the no-cylinder case */
	{ "", },

	/* Size-only metric cylinders */
	{ "10.0ℓ", .ml = 10000 },
	{ "11.1ℓ", .ml = 11100 },

	/* Most common AL cylinders */
	{ "AL40", .cuft = 40, .psi = 3000 },
	{ "AL50", .cuft = 50, .psi = 3000 },
	{ "AL63", .cuft = 63, .psi = 3000 },
	{ "AL72", .cuft = 72, .psi = 3000 },
	{ "AL80", .cuft = 80, .psi = 3000 },
	{ "AL100", .cuft = 100, .psi = 3300 },

	/* Metric AL cylinders */
	{ "ALU7", .ml = 7000, .bar = 200 },

	/* Somewhat common LP steel cylinders */
	{ "LP85", .cuft = 85, .psi = 2640 },
	{ "LP95", .cuft = 95, .psi = 2640 },
	{ "LP108", .cuft = 108, .psi = 2640 },
	{ "LP121", .cuft = 121, .psi = 2640 },

	/* Somewhat common HP steel cylinders */
	{ "HP65", .cuft = 65, .psi = 3442 },
	{ "HP80", .cuft = 80, .psi = 3442 },
	{ "HP100", .cuft = 100, .psi = 3442 },
	{ "HP117", .cuft = 117, .psi = 3442 },
	{ "HP119", .cuft = 119, .psi = 3442 },
	{ "HP130", .cuft = 130, .psi = 3442 },

	/* Common European steel cylinders */
	{ "3ℓ 232 bar", .ml = 3000, .bar = 232 },
	{ "3ℓ 300 bar", .ml = 3000, .bar = 300 },
	{ "10ℓ 200 bar", .ml = 10000, .bar = 200 },
	{ "10ℓ 232 bar", .ml = 10000, .bar = 232 },
	{ "10ℓ 300 bar", .ml = 10000, .bar = 300 },
	{ "12ℓ 200 bar", .ml = 12000, .bar = 200 },
	{ "12ℓ 232 bar", .ml = 12000, .bar = 232 },
	{ "12ℓ 300 bar", .ml = 12000, .bar = 300 },
	{ "15ℓ 200 bar", .ml = 15000, .bar = 200 },
	{ "15ℓ 232 bar", .ml = 15000, .bar = 232 },
	{ "D7 300 bar", .ml = 14000, .bar = 300 },
	{ "D8.5 232 bar", .ml = 17000, .bar = 232 },
	{ "D12 232 bar", .ml = 24000, .bar = 232 },
	{ "D13 232 bar", .ml = 26000, .bar = 232 },
	{ "D15 232 bar", .ml = 30000, .bar = 232 },
	{ "D16 232 bar", .ml = 32000, .bar = 232 },
	{ "D18 232 bar", .ml = 36000, .bar = 232 },
	{ "D20 232 bar", .ml = 40000, .bar = 232 },

	/* We'll fill in more from the dive log dynamically */
	{ NULL, }
};

/*
 * We hardcode the most common weight system types
 * This is a bit odd as the weight system types don't usually encode weight
 */
struct ws_info_t ws_info[MAX_WS_INFO] = {
	{ QT_TRANSLATE_NOOP("gettextFromC", "integrated"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "belt"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "ankle"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "backplate"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "clip-on"), 0 },
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
		fprintf(stderr, "Warning: accessing invalid cylinder %d (%d existing)\n", idx, d->cylinders.nr);
		return NULL;
	}
	return &d->cylinders.cylinders[idx];
}

cylinder_t *get_or_create_cylinder(struct dive *d, int idx)
{
	if (idx < 0) {
		fprintf(stderr, "Warning: accessing invalid cylinder %d\n", idx);
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
	struct tank_info_t *ti = tank_info;
	pressure_t pO2 = {.mbar = lrint(prefs.modpO2 * 1000.0)};

	if (!cyl_name)
		return;
	while (ti->name != NULL && ti < tank_info + MAX_TANK_INFO) {
		if (strcmp(ti->name, cyl_name) == 0)
			break;
		ti++;
	}
	if (ti->name == NULL)
		/* didn't find it */
		return;
	cyl->type.description = strdup(ti->name);
	if (ti->ml) {
		cyl->type.size.mliter = ti->ml;
		cyl->type.workingpressure.mbar = ti->bar * 1000;
	} else {
		cyl->type.workingpressure.mbar = psi_to_mbar(ti->psi);
		if (ti->psi)
			cyl->type.size.mliter = lrint(cuft_to_l(ti->cuft) * 1000 / bar_to_atm(psi_to_bar(ti->psi)));
	}
	// MOD of air
	cyl->depth = gas_mod(cyl->gasmix, pO2, dive, 1);
}

cylinder_t create_new_cylinder(const struct dive *d)
{
	cylinder_t cyl = empty_cylinder;
	fill_default_cylinder(d, &cyl);
	cyl.start = cyl.type.workingpressure;
	cyl.manually_added = true;
	cyl.cylinder_use = OC_GAS;
	return cyl;
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
