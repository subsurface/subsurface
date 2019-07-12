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
#include "subsurface-string.h"
#include "table.h"

static void free_weightsystem(weightsystem_t w)
{
	free((void *)w.description);
}

void copy_weights(const struct weightsystem_table *s, struct weightsystem_table *d)
{
	clear_weightsystem_table(d);
	for (int i = 0; i < s->nr; i++)
		add_cloned_weightsystem(d, s->weightsystems[i]);
}

/* weightsystem table functions */
//static MAKE_GET_IDX(weightsystem_table, weightsystem_t, weightsystems)
static MAKE_GROW_TABLE(weightsystem_table, weightsystem_t, weightsystems)
//static MAKE_GET_INSERTION_INDEX(weightsystem_table, weightsystem_t, weightsystems, weightsystem_less_than)
MAKE_ADD_TO(weightsystem_table, weightsystem_t, weightsystems)
MAKE_REMOVE_FROM(weightsystem_table, weightsystems)
//MAKE_SORT(weightsystem_table, weightsystem_t, weightsystems, comp_weightsystems)
//MAKE_REMOVE(weightsystem_table, weightsystem_t, weightsystem)
MAKE_CLEAR_TABLE(weightsystem_table, weightsystems, weightsystem)

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

/* Add a clone of a weightsystem to the end of a weightsystem table.
 * Cloned in means that the description-string is copied. */
void add_cloned_weightsystem(struct weightsystem_table *t, weightsystem_t ws)
{
	weightsystem_t w_clone = { ws.weight, copy_string(ws.description) };
	add_to_weightsystem_table(t, t->nr, w_clone);
}

bool same_weightsystem(weightsystem_t w1, weightsystem_t w2)
{
	return w1.weight.grams == w2.weight.grams &&
	       same_string(w1.description, w2.description);
}

bool cylinder_nodata(const cylinder_t *cyl)
{
	return !cyl->type.size.mliter &&
	       !cyl->type.workingpressure.mbar &&
	       !cyl->type.description &&
	       !cyl->gasmix.o2.permille &&
	       !cyl->gasmix.he.permille &&
	       !cyl->start.mbar &&
	       !cyl->end.mbar &&
	       !cyl->gas_used.mliter &&
	       !cyl->deco_gas_used.mliter;
}

static bool cylinder_nosamples(const cylinder_t *cyl)
{
	return !cyl->sample_start.mbar &&
	       !cyl->sample_end.mbar;
}

bool cylinder_none(const cylinder_t *cyl)
{
	return cylinder_nodata(cyl) && cylinder_nosamples(cyl);
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

int find_best_gasmix_match(struct gasmix mix, const cylinder_t array[])
{
	int i;
	int best = -1, score = INT_MAX;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		const cylinder_t *match;
		int distance;

		match = array + i;
		if (cylinder_nodata(match))
			continue;
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
	cylinder_t *cyl = dive->cylinder + idx;
	int nr = MAX_CYLINDERS - idx - 1;
	memmove(cyl, cyl + 1, nr * sizeof(*cyl));
	memset(cyl + nr, 0, sizeof(*cyl));
}

void remove_weightsystem(struct dive *dive, int idx)
{
	remove_from_weightsystem_table(&dive->weightsystems, idx);
}

/* when planning a dive we need to make sure that all cylinders have a sane depth assigned
 * and if we are tracking gas consumption the pressures need to be reset to start = end = workingpressure */
void reset_cylinders(struct dive *dive, bool track_gas)
{
	pressure_t decopo2 = {.mbar = prefs.decopo2};

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &dive->cylinder[i];
		if (cylinder_none(cyl))
			continue;
		if (cyl->depth.mm == 0) /* if the gas doesn't give a mod, calculate based on prefs */
			cyl->depth = gas_mod(cyl->gasmix, decopo2, dive, M_OR_FT(3,10));
		if (track_gas)
			cyl->start.mbar = cyl->end.mbar = cyl->type.workingpressure.mbar;
		cyl->gas_used.mliter = 0;
		cyl->deco_gas_used.mliter = 0;
	}
}

#ifdef DEBUG_CYL
void dump_cylinders(struct dive *dive, bool verbose)
{
	printf("Cylinder list:\n");
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &dive->cylinder[i];

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
