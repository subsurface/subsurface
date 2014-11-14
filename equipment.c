/* equipment.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "gettext.h"
#include "dive.h"
#include "display.h"
#include "divelist.h"

/* placeholders for a few functions that we need to redesign for the Qt UI */
void add_cylinder_description(cylinder_type_t *type)
{
	const char *desc;
	int i;

	desc = type->description;
	if (!desc)
		return;
	for (i = 0; i < 100 && tank_info[i].name != NULL; i++) {
		if (strcmp(tank_info[i].name, desc) == 0)
			return;
	}
	if (i < 100) {
		tank_info[i].name = strdup(desc);
		tank_info[i].ml = type->size.mliter;
		tank_info[i].bar = type->workingpressure.mbar / 1000;
	}
}
void add_weightsystem_description(weightsystem_t *weightsystem)
{
	const char *desc;
	int i;

	desc = weightsystem->description;
	if (!desc)
		return;
	for (i = 0; i < 100 && ws_info[i].name != NULL; i++) {
		if (strcmp(ws_info[i].name, desc) == 0) {
			ws_info[i].grams = weightsystem->weight.grams;
			return;
		}
	}
	if (i < 100) {
		ws_info[i].name = strdup(desc);
		ws_info[i].grams = weightsystem->weight.grams;
	}
}

bool cylinder_nodata(cylinder_t *cyl)
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

static bool cylinder_nosamples(cylinder_t *cyl)
{
	return !cyl->sample_start.mbar &&
	       !cyl->sample_end.mbar;
}

bool cylinder_none(void *_data)
{
	cylinder_t *cyl = _data;
	return cylinder_nodata(cyl) && cylinder_nosamples(cyl);
}

void get_gas_string(const struct gasmix *gasmix, char *text, int len)
{
	if (gasmix_is_air(gasmix))
		snprintf(text, len, "%s", translate("gettextFromC", "air"));
	else if (get_he(gasmix) == 0)
		snprintf(text, len, translate("gettextFromC", "EAN%d"), (get_o2(gasmix) + 5) / 10);
	else
		snprintf(text, len, "(%d/%d)", (get_o2(gasmix) + 5) / 10, (get_he(gasmix) + 5) / 10);
}

/* Returns a static char buffer - only good for immediate use by printf etc */
const char *gasname(const struct gasmix *gasmix)
{
	static char gas[64];
	get_gas_string(gasmix, gas, sizeof(gas));
	return gas;
}

bool weightsystem_none(void *_data)
{
	weightsystem_t *ws = _data;
	return !ws->weight.grams && !ws->description;
}

bool no_weightsystems(weightsystem_t *ws)
{
	int i;

	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		if (!weightsystem_none(ws + i))
			return false;
	return true;
}

static bool one_weightsystem_equal(weightsystem_t *ws1, weightsystem_t *ws2)
{
	return ws1->weight.grams == ws2->weight.grams &&
	       same_string(ws1->description, ws2->description);
}

bool weightsystems_equal(weightsystem_t *ws1, weightsystem_t *ws2)
{
	int i;

	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		if (!one_weightsystem_equal(ws1 + i, ws2 + i))
			return false;
	return true;
}

/*
 * We hardcode the most common standard cylinders,
 * we should pick up any other names from the dive
 * logs directly.
 */
struct tank_info_t tank_info[100] = {
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
	{ "HP119", .cuft = 119, .psi = 3442 },
	{ "HP130", .cuft = 130, .psi = 3442 },

	/* Common European steel cylinders */
	{ "3ℓ 232 bar", .ml = 3000, .bar = 232 },
	{ "3ℓ 300 bar", .ml = 3000, .bar = 300 },
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
struct ws_info_t ws_info[100] = {
	{ QT_TRANSLATE_NOOP("gettextFromC", "integrated"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "belt"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "ankle"), 0 },
	{ QT_TRANSLATE_NOOP("gettextFromC", "backplate weight"), 0 },
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
	weightsystem_t *ws = dive->weightsystem + idx;
	int nr = MAX_WEIGHTSYSTEMS - idx - 1;
	memmove(ws, ws + 1, nr * sizeof(*ws));
	memset(ws + nr, 0, sizeof(*ws));
}

/* when planning a dive we need to make sure that all cylinders have a sane depth assigned
 * and if we are tracking gas consumption the pressures need to be reset to start = end = workingpressure */
void reset_cylinders(struct dive *dive, bool track_gas)
{
	int i;
	pressure_t pO2 = {.mbar = 1400};

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &dive->cylinder[i];
		if (cylinder_none(cyl))
			continue;
		if (cyl->depth.mm == 0) /* if the gas doesn't give a mod, assume conservative pO2 */
			cyl->depth = gas_mod(&cyl->gasmix, pO2, M_OR_FT(3,10));
		if (track_gas)
			cyl->start.mbar = cyl->end.mbar = cyl->type.workingpressure.mbar;
		cyl->gas_used.mliter = 0;
		cyl->deco_gas_used.mliter = 0;
	}
}
