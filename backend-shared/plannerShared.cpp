// SPDX-License-Identifier: GPL-2.0
#include "plannerShared.h"
#include "core/pref.h"
#include "core/settings/qPrefDivePlanner.h"


plannerShared *plannerShared::instance()
{
    static plannerShared *self = new plannerShared;
    return self;
}

// Used to convert between meter/feet and keep the qPref variables independent
#define TO_MM_BY_SEC ((prefs.units.length == units::METERS) ? 1000.0 / 60.0 : feet_to_mm(1.0) / 60.0)

// Converted meter/feet qPrefDivePlanner values
int plannerShared::ascratelast6m()
{
	return lrint(prefs.ascratelast6m / TO_MM_BY_SEC);
}
void plannerShared::set_ascratelast6m(int value)
{
	qPrefDivePlanner::set_ascratelast6m(value * TO_MM_BY_SEC);
}

int plannerShared::ascratestops()
{
	return lrint(prefs.ascratestops / TO_MM_BY_SEC);
}
void plannerShared::set_ascratestops(int value)
{
	qPrefDivePlanner::set_ascratestops(value * TO_MM_BY_SEC);
}

int plannerShared::ascrate50()
{
	return lrint(prefs.ascrate50 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate50(int value)
{
	qPrefDivePlanner::set_ascrate50(value * TO_MM_BY_SEC);
}

int plannerShared::ascrate75()
{
	return lrint(prefs.ascrate75 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate75(int value)
{
	qPrefDivePlanner::set_ascrate75(value * TO_MM_BY_SEC);
}

int plannerShared::descrate()
{
	return lrint(prefs.descrate / TO_MM_BY_SEC);
}
void plannerShared::set_descrate(int value)
{
	qPrefDivePlanner::set_descrate(value * TO_MM_BY_SEC);
}


// Planning values
deco_mode plannerShared::planner_deco_mode()
{
	return BUEHLMANN;
}
void plannerShared::set_planner_deco_mode(deco_mode value)
{
//	qPrefDivePlanner::set_X(value);
}

bool plannerShared::dobailout()
{
	return 1;
}
void plannerShared::set_dobailout(bool value)
{
	//	qPrefDivePlanner::set_X(value);
}

int plannerShared::reserve_gas()
{
	return 1;
}
void plannerShared::set_reserve_gas(int value)
{
	//	qPrefDivePlanner::set_X(value);
}

bool plannerShared::safetystop()
{
	return 1;
}
void plannerShared::set_safetystop(bool value)
{
	//	qPrefDivePlanner::set_X(value);
}

int plannerShared::gflow()
{
	return 1;
}
void plannerShared::set_gflow(int value)
{
	//	qPrefDivePlanner::set_X(value);
}

int plannerShared::gfhigh()
{
	return 1;
}
void plannerShared::set_gfhigh(int value)
{
	//	qPrefDivePlanner::set_X(value);
}

int plannerShared::vpmb_conservatism()
{
	return 1;
}
void plannerShared::set_vpmb_conservatism(int value)
{
	//	qPrefDivePlanner::set_X(value);
}
