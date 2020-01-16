// SPDX-License-Identifier: GPL-2.0
#include "plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefUnit.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/cylindermodel.h"
#include <QDebug>

plannerShared *plannerShared::instance()
{
    static plannerShared *self = new plannerShared;
    return self;
}
plannerShared::plannerShared()
{
	// Be informed when user switches METER <-> FEET and LITER <-> CUFT
	connect(qPrefUnits::instance(), &qPrefUnits::lengthChanged, this, &unit_lengthChangedSlot);
	connect(qPrefUnits::instance(), &qPrefUnits::volumeChanged, this, &unit_volumeChangedSlot);
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
	emit instance()->ascratelast6mChanged(value);
}

int plannerShared::ascratestops()
{
	return lrint((float)prefs.ascratestops / TO_MM_BY_SEC);
}
void plannerShared::set_ascratestops(int value)
{
	qPrefDivePlanner::set_ascratestops(lrint((float)value * TO_MM_BY_SEC));
	emit instance()->ascratestopsChanged(value);
}

int plannerShared::ascrate50()
{
	return lrint((float)prefs.ascrate50 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate50(int value)
{
	qPrefDivePlanner::set_ascrate50(lrint((float)value * TO_MM_BY_SEC));
	emit instance()->ascrate50Changed(value);
}

int plannerShared::ascrate75()
{
	return lrint((float)prefs.ascrate75 / TO_MM_BY_SEC);
}
void plannerShared::set_ascrate75(int value)
{
	qPrefDivePlanner::set_ascrate75(lrint((float)value * TO_MM_BY_SEC));
	emit instance()->ascrate75Changed(value);
}

int plannerShared::descrate()
{
	return lrint((float)prefs.descrate / TO_MM_BY_SEC);
}
void plannerShared::set_descrate(int value)
{
	qPrefDivePlanner::set_descrate(lrint((float)value * TO_MM_BY_SEC));
	emit instance()->descrateChanged(value);
}

// Planning values
int plannerShared::planner_deco_mode()
{
	return (int)qPrefDivePlanner::planner_deco_mode();
}
void plannerShared::set_planner_deco_mode(int value)
{
	DivePlannerPointsModel::instance()->setDecoMode((deco_mode)value);
	emit instance()->planner_deco_modeChanged(value);
}

// TBD, this needs to be integrated deeper into the planner model
int plannerShared::divemode = OC;

int plannerShared::dive_mode()
{
	return divemode;
}
void plannerShared::set_dive_mode(int value)
{
	divemode = value;
	DivePlannerPointsModel::instance()->setRebreatherMode(value);
	emit instance()->dive_modeChanged(value);
}

int plannerShared::reserve_gas()
{
	return qPrefUnits::pressure() == units::BAR ? prefs.reserve_gas / 1000 :
												  mbar_to_PSI(prefs.reserve_gas);
}
void plannerShared::set_reserve_gas(int value)
{
	DivePlannerPointsModel::instance()->setReserveGas(value);
	emit instance()->reserve_gasChanged(reserve_gas());
}

bool plannerShared::safetystop()
{
	return qPrefDivePlanner::safetystop();
}
void plannerShared::set_safetystop(bool value)
{
	DivePlannerPointsModel::instance()->setSafetyStop(value);
	emit instance()->safetystopChanged(value);
}

int plannerShared::gflow()
{
	return qPrefTechnicalDetails::gflow();
}
void plannerShared::set_gflow(int value)
{
	DivePlannerPointsModel::instance()->setGFLow(value);
	qPrefTechnicalDetails::set_gflow(value);
	emit instance()->gflowChanged(value);
}

int plannerShared::gfhigh()
{
	return qPrefTechnicalDetails::gflow();
}
void plannerShared::set_gfhigh(int value)
{
	DivePlannerPointsModel::instance()->setGFHigh(value);
	qPrefTechnicalDetails::set_gflow(value);
	emit instance()->gfhighChanged(value);
}

int plannerShared::vpmb_conservatism()
{
	return qPrefTechnicalDetails::vpmb_conservatism();
}
void plannerShared::set_vpmb_conservatism(int value)
{
	DivePlannerPointsModel::instance()->setVpmbConservatism(value);
	qPrefTechnicalDetails::set_vpmb_conservatism(value);
	emit instance()->vpmb_conservatismChanged(value);
}

bool plannerShared::dobailout()
{
	return qPrefDivePlanner::dobailout();
}
void plannerShared::set_dobailout(bool value)
{
	qPrefDivePlanner::set_dobailout(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
	emit instance()->dobailoutChanged(value);
}

bool plannerShared::drop_stone_mode()
{
	return qPrefDivePlanner::drop_stone_mode();
}
void plannerShared::set_drop_stone_mode(bool value)
{
	DivePlannerPointsModel::instance()->setDropStoneMode(value);
	emit instance()->drop_stone_modeChanged(value);
}

bool plannerShared::last_stop()
{
	return qPrefDivePlanner::last_stop();
}
void plannerShared::set_last_stop(bool value)
{
	DivePlannerPointsModel::instance()->setLastStop6m(value);
	emit instance()->last_stopChanged(value);
}

bool plannerShared::switch_at_req_stop()
{
	return qPrefDivePlanner::switch_at_req_stop();
}
void plannerShared::set_switch_at_req_stop(bool value)
{
	DivePlannerPointsModel::instance()->setSwitchAtReqStop(value);
	emit instance()->switch_at_req_stopChanged(value);
}

bool plannerShared::doo2breaks()
{
	return qPrefDivePlanner::doo2breaks();
}
void plannerShared::set_doo2breaks(bool value)
{
	qPrefDivePlanner::set_doo2breaks(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
	emit instance()->doo2breaksChanged(value);
}

int plannerShared::min_switch_duration()
{
	return qPrefDivePlanner::min_switch_duration() / 60;
}
void plannerShared::set_min_switch_duration(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setMinSwitchDuration(value);
	emit instance()->min_switch_durationChanged(value);
}

int plannerShared::surface_segment()
{
	return qPrefDivePlanner::surface_segment() / 60;
}
void plannerShared::set_surface_segment(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setSurfaceSegment(value);
	emit instance()->surface_segmentChanged(surface_segment());
}

double plannerShared::bottomsac()
{
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::bottomsac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::bottomsac());
}
void plannerShared::set_bottomsac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setBottomSac(value);
	emit instance()->bottomsacChanged(bottomsac());
}

double plannerShared::decosac()
{
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::decosac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::decosac());
}
void plannerShared::set_decosac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setDecoSac(value);
	emit instance()->decosacChanged(decosac());
}

int plannerShared::problemsolvingtime()
{
	return qPrefDivePlanner::problemsolvingtime();
}
void plannerShared::set_problemsolvingtime(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setProblemSolvingTime(value);
	emit instance()->problemsolvingtimeChanged(value);
}

double plannerShared::sacfactor()
{
	return qPrefDivePlanner::sacfactor() / 100.0;
}
void plannerShared::set_sacfactor(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setSacFactor(value);
	emit instance()->sacfactorChanged(value);
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
	emit instance()->o2narcoticChanged(value);
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
	emit instance()->bottompo2Changed(value);
}

double plannerShared::decopo2()
{
	return qPrefDivePlanner::decopo2() / 1000.0;
}
void plannerShared::set_decopo2(double value)
{
	pressure_t olddecopo2;
	olddecopo2.mbar = prefs.decopo2;
	qPrefDivePlanner::instance()->set_decopo2((int) (value * 1000.0));
	CylindersModel::instance()->updateDecoDepths(olddecopo2);
	CylindersModel::instance()->updateBestMixes();
	emit instance()->decopo2Changed(value);
}

int plannerShared::bestmixend()
{
	return lrint(get_depth_units(prefs.bestmixend.mm, NULL, NULL));
}
void plannerShared::set_bestmixend(int value)
{
	qPrefDivePlanner::set_bestmixend(units_to_depth(value).mm);
	CylindersModel::instance()->updateBestMixes();
	emit instance()->bestmixendChanged(value);
}

bool plannerShared::display_runtime()
{
	return qPrefDivePlanner::display_runtime();
}
void plannerShared::set_display_runtime(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayRuntime(value);
	emit instance()->display_runtimeChanged(value);
}

bool plannerShared::display_duration()
{
	return qPrefDivePlanner::display_duration();
}
void plannerShared::set_display_duration(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayDuration(value);
	emit instance()->display_durationChanged(value);
}

bool plannerShared::display_transitions()
{
	return qPrefDivePlanner::display_transitions();
}
void plannerShared::set_display_transitions(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayTransitions(value);
	emit instance()->display_transitionsChanged(value);
}

bool plannerShared::verbatim_plan()
{
	return qPrefDivePlanner::verbatim_plan();
}
void plannerShared::set_verbatim_plan(bool value)
{
	DivePlannerPointsModel::instance()->setVerbatim(value);
	emit instance()->verbatim_planChanged(value);
}

bool plannerShared::display_variations()
{
	return qPrefDivePlanner::display_variations();
}
void plannerShared::set_display_variations(bool value)
{
	DivePlannerPointsModel::instance()->setDisplayVariations(value);
	emit instance()->display_variationsChanged(value);
}

// Handle when user changes length measurement type
void plannerShared::unit_lengthChangedSlot(int value)
{
	// Provoke recalculation of model and send of signals
	set_ascratelast6m(ascratelast6m());
	set_ascratestops(ascratestops());
	set_ascrate50(ascrate50());
	set_ascrate75(ascrate75());
	set_descrate(descrate());
	set_bestmixend(bestmixend());
}

// Handle when user changes volume measurement type
void plannerShared::unit_volumeChangedSlot(int value)
{
	// Provoke recalculation of model and send of signals
	set_bottomsac(bottomsac());
	set_decosac(decosac());
}
