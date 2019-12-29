// SPDX-License-Identifier: GPL-2.0
#include "plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/cylindermodel.h"

plannerShared *plannerShared::instance()
{
    static plannerShared *self = new plannerShared;
    return self;
}

// Used to convert between meter/feet and keep the qPref variables independent
#define TO_MM_BY_SEC ((prefs.units.length == units::METERS) ? 1000.0 / 60.0 : feet_to_mm(1) / 60.0)

// Converted meter/feet qPrefDivePlanner values
int plannerShared::ascratelast6m()
{
	return lrint((float)prefs.ascratelast6m / TO_MM_BY_SEC);
}
void plannerShared::set_ascratelast6m(int value)
{
	qPrefDivePlanner::set_ascratelast6m(lrint((float)value * TO_MM_BY_SEC));
}

int plannerShared::ascratestops()
{
	return lrint((float)prefs.ascratestops / TO_MM_BY_SEC);
}
void plannerShared::set_ascratestops(int value)
{
	qPrefDivePlanner::set_ascratestops(lrint((float)value * TO_MM_BY_SEC));
}

int plannerShared::ascrate50()
{
	return lrint((float)prefs.ascrate50 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate50(int value)
{
	qPrefDivePlanner::set_ascrate50(lrint((float)value * TO_MM_BY_SEC));
}

int plannerShared::ascrate75()
{
	return lrint((float)prefs.ascrate75 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate75(int value)
{
	qPrefDivePlanner::set_ascrate75(lrint((float)value * TO_MM_BY_SEC));
}

int plannerShared::descrate()
{
	return lrint((float)prefs.descrate / TO_MM_BY_SEC);
}
void plannerShared::set_descrate(int value)
{
	qPrefDivePlanner::set_descrate(lrint((float)value * TO_MM_BY_SEC));
}

// Planning values
deco_mode plannerShared::planner_deco_mode()
{
	return qPrefDivePlanner::planner_deco_mode();
}
void plannerShared::set_planner_deco_mode(deco_mode value)
{
	DivePlannerPointsModel::instance()->setDecoMode(value);
}

int plannerShared::reserve_gas()
{
	return qPrefDivePlanner::reserve_gas();
}
void plannerShared::set_reserve_gas(int value)
{
	DivePlannerPointsModel::instance()->setReserveGas(value);
}

bool plannerShared::safetystop()
{
	return qPrefDivePlanner::safetystop();
}
void plannerShared::set_safetystop(bool value)
{
	DivePlannerPointsModel::instance()->setSafetyStop(value);
}

int plannerShared::gflow()
{
	return qPrefTechnicalDetails::gflow();
}
void plannerShared::set_gflow(int value)
{
	DivePlannerPointsModel::instance()->setGFLow(value);
}

int plannerShared::gfhigh()
{
	return qPrefTechnicalDetails::gflow();
}
void plannerShared::set_gfhigh(int value)
{
	DivePlannerPointsModel::instance()->setGFHigh(value);
}

int plannerShared::vpmb_conservatism()
{
	return qPrefTechnicalDetails::vpmb_conservatism();
}
void plannerShared::set_vpmb_conservatism(int value)
{
	DivePlannerPointsModel::instance()->setVpmbConservatism(value);
}

bool plannerShared::dobailout()
{
	return qPrefDivePlanner::dobailout();
}
void plannerShared::set_dobailout(bool value)
{
	qPrefDivePlanner::set_dobailout(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
}

bool plannerShared::drop_stone_mode()
{
	return qPrefDivePlanner::drop_stone_mode();
}
void plannerShared::set_drop_stone_mode(bool value)
{
	DivePlannerPointsModel::instance()->setDropStoneMode(value);
}

bool plannerShared::last_stop()
{
	return qPrefDivePlanner::last_stop();
}
void plannerShared::set_last_stop(bool value)
{
	DivePlannerPointsModel::instance()->setLastStop6m(value);
}

bool plannerShared::switch_at_req_stop()
{
	return qPrefDivePlanner::switch_at_req_stop();
}
void plannerShared::set_switch_at_req_stop(bool value)
{
	DivePlannerPointsModel::instance()->setSwitchAtReqStop(value);
}

bool plannerShared::doo2breaks()
{
	return qPrefDivePlanner::doo2breaks();
}
void plannerShared::set_doo2breaks(bool value)
{
	qPrefDivePlanner::set_doo2breaks(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
}

int plannerShared::min_switch_duration()
{
	return qPrefDivePlanner::min_switch_duration() / 60;
}
void plannerShared::set_min_switch_duration(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setMinSwitchDuration(value);
}

double plannerShared::bottomsac()
{
	return qPrefDivePlanner::bottomsac() / 1000.0;
}
void plannerShared::set_bottomsac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setBottomSac(value);
}

double plannerShared::decosac()
{
	return qPrefDivePlanner::decosac() / 1000.0;
}
void plannerShared::set_decosac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setDecoSac(value);
}

int plannerShared::problemsolvingtime()
{
	return qPrefDivePlanner::problemsolvingtime();
}
void plannerShared::set_problemsolvingtime(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setProblemSolvingTime(value);
}

double plannerShared::sacfactor()
{
	return qPrefDivePlanner::sacfactor() / 100.0;
}
void plannerShared::set_sacfactor(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setSacFactor(value);
}

bool plannerShared::o2narcotic()
{
	return qPrefDivePlanner::o2narcotic();
}
void plannerShared::set_o2narcotic(bool value)
{
	qPrefDivePlanner::set_o2narcotic(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
	CylindersModel::instance()->updateBestMixes();
}

double plannerShared::bottompo2()
{
	return (qPrefDivePlanner::bottompo2() / 1000.0);
}
void plannerShared::set_bottompo2(double value)
{
	// NO conversion, this is done in the planner model.
	qPrefDivePlanner::set_bottompo2((int) (value * 1000.0));
	CylindersModel::instance()->updateBestMixes();
}

double plannerShared::decopo2()
{
	return qPrefDivePlanner::decopo2() / 1000;
}
void plannerShared::set_decopo2(double value)
{
	pressure_t olddecopo2;
	olddecopo2.mbar = prefs.decopo2;
	qPrefDivePlanner::instance()->set_decopo2((int) (value * 1000.0));
	CylindersModel::instance()->updateDecoDepths(olddecopo2);
	CylindersModel::instance()->updateBestMixes();
}

int plannerShared::bestmixend()
{
	return lrint(get_depth_units(prefs.bestmixend.mm, NULL, NULL));
}
void plannerShared::set_bestmixend(int value)
{
	qPrefDivePlanner::set_bestmixend(units_to_depth(value).mm);
	CylindersModel::instance()->updateBestMixes();
}

bool plannerShared::display_runtime()
{
	return qPrefDivePlanner::display_runtime();
}
void plannerShared::set_display_runtime(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayRuntime(value);
}

bool plannerShared::display_duration()
{
	return qPrefDivePlanner::display_duration();
}
void plannerShared::set_display_duration(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayDuration(value);
}

bool plannerShared::display_transitions()
{
	return qPrefDivePlanner::display_transitions();
}
void plannerShared::set_display_transitions(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayTransitions(value);
}

bool plannerShared::verbatim_plan()
{
	return qPrefDivePlanner::verbatim_plan();
}
void plannerShared::set_verbatim_plan(bool value)
{
	DivePlannerPointsModel::instance()->setVerbatim(value);
}

bool plannerShared::display_variations()
{
	return qPrefDivePlanner::display_variations();
}
void plannerShared::set_display_variations(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayVariations(value);
}
