// SPDX-License-Identifier: GPL-2.0
#include "core/subsurface-string.h"
#include "qPrefDivePlanner.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("Planner");

qPrefDivePlanner *qPrefDivePlanner::instance()
{
	static qPrefDivePlanner *self = new qPrefDivePlanner;
	return self;
}



void qPrefDivePlanner::loadSync(bool doSync)
{
	disk_ascratelast6m(doSync);
	disk_ascratestops(doSync);
	disk_ascrate50(doSync);
	disk_ascrate75(doSync);
	disk_bestmixend(doSync);
	disk_bottompo2(doSync);
	disk_bottomsac(doSync);
	disk_decopo2(doSync);
	disk_decosac(doSync);
	disk_descrate(doSync);
	disk_display_duration(doSync);
	disk_display_runtime(doSync);
	disk_display_transitions(doSync);
	disk_display_variations(doSync);
	disk_doo2breaks(doSync);
	disk_dobailout(doSync);
	disk_o2narcotic(doSync);
	disk_drop_stone_mode(doSync);
	disk_last_stop(doSync);
	disk_min_switch_duration(doSync);
	disk_planner_deco_mode(doSync);
	disk_problemsolvingtime(doSync);
	disk_reserve_gas(doSync);
	disk_sacfactor(doSync);
	disk_safetystop(doSync);
	disk_switch_at_req_stop(doSync);
	disk_verbatim_plan(doSync);
}

HANDLE_PREFERENCE_INT(DivePlanner, "ascratelast6m", ascratelast6m);

HANDLE_PREFERENCE_INT(DivePlanner, "ascratestops", ascratestops);

HANDLE_PREFERENCE_INT(DivePlanner, "ascrate50", ascrate50);

HANDLE_PREFERENCE_INT(DivePlanner, "ascrate75", ascrate75);

HANDLE_PREFERENCE_STRUCT(DivePlanner, "bestmixend", bestmixend, mm);

HANDLE_PREFERENCE_INT(DivePlanner, "bottompo2", bottompo2);

HANDLE_PREFERENCE_INT(DivePlanner, "bottomsac", bottomsac);

HANDLE_PREFERENCE_INT(DivePlanner, "decopo2", decopo2);

HANDLE_PREFERENCE_INT(DivePlanner, "decosac", decosac);

HANDLE_PREFERENCE_INT(DivePlanner, "descrate", descrate);

HANDLE_PREFERENCE_BOOL(DivePlanner, "display_duration", display_duration);

HANDLE_PREFERENCE_BOOL(DivePlanner, "display_runtime", display_runtime);

HANDLE_PREFERENCE_BOOL(DivePlanner, "display_transitions", display_transitions);
HANDLE_PREFERENCE_BOOL(DivePlanner, "display_variations", display_variations);

HANDLE_PREFERENCE_BOOL(DivePlanner, "doo2breaks", doo2breaks);
HANDLE_PREFERENCE_BOOL(DivePlanner, "dobailbout", dobailout);
HANDLE_PREFERENCE_BOOL(DivePlanner, "o2narcotic", o2narcotic);

HANDLE_PREFERENCE_BOOL(DivePlanner, "drop_stone_mode", drop_stone_mode);

HANDLE_PREFERENCE_BOOL(DivePlanner, "last_stop", last_stop);

HANDLE_PREFERENCE_INT(DivePlanner, "min_switch_duration", min_switch_duration);

HANDLE_PREFERENCE_INT(DivePlanner, "surface_segment", surface_segment);

HANDLE_PREFERENCE_ENUM(DivePlanner, deco_mode, "deco_mode", planner_deco_mode);

HANDLE_PREFERENCE_INT(DivePlanner, "problemsolvingtime", problemsolvingtime);

HANDLE_PREFERENCE_INT(DivePlanner, "reserve_gas", reserve_gas);

HANDLE_PREFERENCE_INT(DivePlanner, "sacfactor", sacfactor);

HANDLE_PREFERENCE_BOOL(DivePlanner, "safetystop", safetystop);

HANDLE_PREFERENCE_BOOL(DivePlanner, "switch_at_req_stop", switch_at_req_stop);

HANDLE_PREFERENCE_BOOL(DivePlanner, "verbatim_plan", verbatim_plan);
