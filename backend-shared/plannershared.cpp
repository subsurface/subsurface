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

