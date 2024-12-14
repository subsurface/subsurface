// SPDX-License-Identifier: GPL-2.0
#include "plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefUnit.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/cylindermodel.h"

// Planning values
deco_mode PlannerShared::planner_deco_mode()
{
	return qPrefDivePlanner::planner_deco_mode();
}
void PlannerShared::set_planner_deco_mode(deco_mode value)
{
	DivePlannerPointsModel::instance()->setDecoMode(value);
}

int PlannerShared::reserve_gas()
{
	return qPrefDivePlanner::reserve_gas();
}
void PlannerShared::set_reserve_gas(int value)
{
	DivePlannerPointsModel::instance()->setReserveGas(value);
}

bool PlannerShared::dobailout()
{
	return qPrefDivePlanner::dobailout();
}
void PlannerShared::set_dobailout(bool value)
{
	qPrefDivePlanner::set_dobailout(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
}

bool PlannerShared::doo2breaks()
{
	return qPrefDivePlanner::doo2breaks();
}
void PlannerShared::set_doo2breaks(bool value)
{
	qPrefDivePlanner::set_doo2breaks(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
}

int PlannerShared::min_switch_duration()
{
	return qPrefDivePlanner::min_switch_duration() / 60;
}
void PlannerShared::set_min_switch_duration(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setMinSwitchDuration(value);
}

int PlannerShared::surface_segment()
{
	return qPrefDivePlanner::surface_segment() / 60;
}
void PlannerShared::set_surface_segment(int value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setSurfaceSegment(value);
}

double PlannerShared::bottomsac()
{
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::bottomsac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::bottomsac()
#ifdef SUBSURFACE_MOBILE
					* 100 // cuft without decimals (0 - 300)
#endif
				);
}
void PlannerShared::set_bottomsac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setBottomSac(value);
}

double PlannerShared::decosac()
{
// Mobile and desktop use the same values when using units::LITER,
// however when using units::CUFT desktop want 0.00 - 3.00 while
// mobile wants 0 - 300, therefore multiply by 100 for mobile
	return (qPrefUnits::volume() == units::LITER) ?
				qPrefDivePlanner::decosac() / 1000.0 :
				ml_to_cuft(qPrefDivePlanner::decosac()
#ifdef SUBSURFACE_MOBILE
					* 100 // cuft without decimals (0 - 300)
#endif
				);
}
void PlannerShared::set_decosac(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setDecoSac(value);
}

double PlannerShared::sacfactor()
{
// mobile want 0 - 100 which are shown with 1 decimal as 0.0 - 10.0
// whereas desktop wants 0.0 - 10.0
// as a consequence divide by 10 or 100
	return qPrefDivePlanner::sacfactor() /
#ifdef SUBSURFACE_MOBILE
			10.0;
#else
			100.0;
#endif
}
void PlannerShared::set_sacfactor(double value)
{
	// NO conversion, this is done in the planner model.
	DivePlannerPointsModel::instance()->setSacFactor(value);
}

bool PlannerShared::o2narcotic()
{
	return qPrefDivePlanner::o2narcotic();
}
void PlannerShared::set_o2narcotic(bool value)
{
	qPrefDivePlanner::set_o2narcotic(value);
	DivePlannerPointsModel::instance()->emitDataChanged();
	DivePlannerPointsModel::instance()->cylindersModel()->emitDataChanged();
}

double PlannerShared::bottompo2()
{
	return (qPrefDivePlanner::bottompo2() / 1000.0);
}
void PlannerShared::set_bottompo2(double value)
{
	qPrefDivePlanner::set_bottompo2((int) (value * 1000.0));
	DivePlannerPointsModel::instance()->cylindersModel()->updateBestMixes();
}

double PlannerShared::decopo2()
{
	return qPrefDivePlanner::decopo2() / 1000.0;
}
void PlannerShared::set_decopo2(double value)
{
	pressure_t olddecopo2;
	olddecopo2.mbar = prefs.decopo2;
	qPrefDivePlanner::instance()->set_decopo2((int) (value * 1000.0));
	DivePlannerPointsModel::instance()->cylindersModel()->updateDecoDepths(olddecopo2);
	DivePlannerPointsModel::instance()->cylindersModel()->updateBestMixes();
}

int PlannerShared::bestmixend()
{
	return lrint(get_depth_units(prefs.bestmixend, NULL, NULL));
}
void PlannerShared::set_bestmixend(int value)
{
	qPrefDivePlanner::set_bestmixend(units_to_depth(value).mm);
	DivePlannerPointsModel::instance()->cylindersModel()->updateBestMixes();
}
