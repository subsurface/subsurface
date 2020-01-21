// SPDX-License-Identifier: GPL-2.0
#include "plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefUnit.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/cylindermodel.h"

plannerShared *plannerShared::instance()
{
    static plannerShared *self = new plannerShared;
    return self;
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

bool plannerShared::dobailout()
{
	return qPrefDivePlanner::dobailout();
}
void plannerShared::set_dobailout(bool value)
{
	qPrefDivePlanner::set_dobailout(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
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
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::bottomsac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::bottomsac()
#ifdef SUBSURFACE_MOBILE
					* 100 // cuft without decimals (0 - 300)
#endif
				);
}
void plannerShared::set_bottomsac(double value)
{
#ifdef SUBSURFACE_MOBILE
	if (qPrefUnits::volume() == units::CUFT)
		value /= 100; // cuft without decimals (0 - 300)
#endif

	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setBottomSac(value);
}

double plannerShared::decosac()
{
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::decosac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::decosac()
#ifdef SUBSURFACE_MOBILE
					* 100 // cuft without decimals (0 - 300)
#endif
				);
}
void plannerShared::set_decosac(double value)
{
#ifdef SUBSURFACE_MOBILE
	if (qPrefUnits::volume() == units::CUFT)
		value /= 100; // cuft without decimals (0 - 300)
#endif

	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setDecoSac(value);
}

double plannerShared::sacfactor()
{
	return qPrefDivePlanner::sacfactor() /
#ifdef SUBSURFACE_MOBILE
			10.0;
#else
			100.0;
#endif
}
void plannerShared::set_sacfactor(double value)
{
#ifdef SUBSURFACE_MOBILE
	value /= 10.0;
#endif
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
	return qPrefDivePlanner::decopo2() / 1000.0;
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
