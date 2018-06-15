// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPrefPlanner.h"

#include <QSettings>


qPrefPlanner *qPrefPlanner::m_instance = NULL;
qPrefPlanner *qPrefPlanner::instance()
{
	if (!m_instance)
		m_instance = new qPrefPlanner;
	return m_instance;
}



void qPrefPlanner::loadSync(bool doSync)
{
	diskAscRateLast6m(doSync);
	diskAscRateStops(doSync);
	diskAscRate50(doSync);
	diskAscRate75(doSync);
	diskBestmixEnd(doSync);
	diskBottomPo2(doSync);
	diskBottomSac(doSync);
	diskDecoMode(doSync);
	diskDecoPo2(doSync);
	diskDecoSac(doSync);
	diskDescRate(doSync);
	diskDisplayDuration(doSync);
	diskDisplayRuntime(doSync);
	diskDisplayTransitions(doSync);
	diskDisplayVariations(doSync);
	diskDoo2Breaks(doSync);
	diskDropStoneMode(doSync);
	diskLastStop(doSync);
	diskMinSwitchDuration(doSync);
	diskProblemSolvingTime(doSync);
	diskReserveGas(doSync);
	diskSacFactor(doSync);
	diskSafetyStop(doSync);
	diskSwitchAtRequiredStop(doSync);
	diskVerbatimPlan(doSync);
}


int qPrefPlanner::ascRateLast6m() const
{
	return prefs.ascratelast6m;
}
void qPrefPlanner::setAscRateLast6m(int value)
{
	if (value != prefs.ascratelast6m) {
		prefs.ascratelast6m = value;
		diskAscRateLast6m(true);
		emit ascRateLast6mChanged(value);

	}
}
void qPrefPlanner::diskAscRateLast6m(bool doSync)
{
	LOADSYNC_INT("/ascratelast6m", ascratelast6m);
}


int qPrefPlanner::ascRateStops() const
{
	return prefs.ascratestops;
}
void qPrefPlanner::setAscRateStops(int value)
{
	if (value != prefs.ascratestops) {
		prefs.ascratestops = value;
		diskAscRateStops(true);
		emit ascRateStopsChanged(value);
	}
}
void qPrefPlanner::diskAscRateStops(bool doSync)
{
	LOADSYNC_INT("/ascratestops", ascratestops);
}


int qPrefPlanner::ascRate50() const
{
	return prefs.ascrate50;
}
void qPrefPlanner::setAscRate50(int value)
{
	if (value != prefs.ascrate50) {
		prefs.ascrate50 = value;
		diskAscRate50(true);
		emit ascRate50Changed(value);
	}
}
void qPrefPlanner::diskAscRate50(bool doSync)
{
	LOADSYNC_INT("/ascrate50", ascrate50);
}


int qPrefPlanner::ascRate75() const
{
	return prefs.ascrate75;
}
void qPrefPlanner::setAscRate75(int value)
{
	if (value != prefs.ascrate75) {
		prefs.ascrate75 = value;
		diskAscRate75(true);
		emit ascRate75Changed(value);
	}
}
void qPrefPlanner::diskAscRate75(bool doSync)
{
	LOADSYNC_INT("/ascrate75", ascrate75);
}


int qPrefPlanner::bestmixEnd() const
{
	return prefs.bestmixend.mm;
}
void qPrefPlanner::setBestmixEnd(int value)
{
	if (value != prefs.bestmixend.mm) {
		prefs.bestmixend.mm = value;
		diskBestmixEnd(true);
	}
}
void qPrefPlanner::diskBestmixEnd(bool doSync)
{
	LOADSYNC_INT("/bestmixend", bestmixend.mm);
}

int qPrefPlanner::bottomPo2() const
{
	return prefs.bottompo2;
}
void qPrefPlanner::setBottomPo2(int value)
{
	if (value != prefs.bottompo2) {
		prefs.bottompo2 = value;
		diskBottomPo2(true);
		emit bottomPo2Changed(value);
	}
}
void qPrefPlanner::diskBottomPo2(bool doSync)
{
	LOADSYNC_INT("/bottompo2", bottompo2);
}


int qPrefPlanner::bottomSac() const
{
	return prefs.bottomsac;
}
void qPrefPlanner::setBottomSac(int value)
{
	if (value != prefs.bottomsac) {
		prefs.bottomsac = value;
		diskBottomSac(true);
	}
}
void qPrefPlanner::diskBottomSac(bool doSync)
{
	LOADSYNC_INT("/bottomsac", bottomsac);
}

deco_mode qPrefPlanner::decoMode() const
{
	return prefs.planner_deco_mode;
}
void qPrefPlanner::setDecoMode(deco_mode value)
{
	if (value != prefs.planner_deco_mode) {
		prefs.planner_deco_mode = deco_mode(default_prefs.planner_deco_mode);
		diskDecoMode(true);
		emit decoModeChanged(value);

	}
}
void qPrefPlanner::diskDecoMode(bool doSync)
{
	LOADSYNC_ENUM("/deco_mode", deco_mode, planner_deco_mode);
}


int qPrefPlanner::decoPo2() const
{
	return prefs.decopo2;
}
void qPrefPlanner::setDecoPo2(int value)
{
	if (value != prefs.decopo2) {
		prefs.decopo2 = value;
		diskDecoPo2(true);
		emit decoPo2Changed(value);
	}
}
void qPrefPlanner::diskDecoPo2(bool doSync)
{
	LOADSYNC_INT("/decopo2", decopo2);
}


int qPrefPlanner::decoSac() const
{
	return prefs.decosac;
}
void qPrefPlanner::setDecoSac(int value)
{
	if (value != prefs.decosac) {
		prefs.decosac = value;
		diskDecoSac(true);
		emit decoSacChanged(value);
	}
}
void qPrefPlanner::diskDecoSac(bool doSync)
{
	LOADSYNC_INT("/decosac", decosac);
}


int qPrefPlanner::descRate() const
{
	return prefs.descrate;
}
void qPrefPlanner::setDescRate(int value)
{
	if (value != prefs.descrate) {
		prefs.descrate = value;
		diskDescRate(true);
		emit descRateChanged(value);
	}
}
void qPrefPlanner::diskDescRate(bool doSync)
{
	LOADSYNC_INT("/descrate", descrate);
}


bool qPrefPlanner::displayDuration() const
{
	return prefs.display_duration;
}
void qPrefPlanner::setDisplayDuration(bool value)
{
	if (value != prefs.display_duration) {
		prefs.display_duration = value;
		diskDisplayDuration(true);
		emit displayDurationChanged(value);
	}
}
void qPrefPlanner::diskDisplayDuration(bool doSync)
{
	LOADSYNC_BOOL("/display_duration", display_duration);
}


bool qPrefPlanner::displayRuntime() const
{
	return prefs.display_runtime;
}
void qPrefPlanner::setDisplayRuntime(bool value)
{
	if (value != prefs.display_runtime) {
		prefs.display_runtime = value;
		diskDisplayRuntime(true);
		emit displayRuntimeChanged(value);
	}
}
void qPrefPlanner::diskDisplayRuntime(bool doSync)
{
	LOADSYNC_BOOL("/display_runtime", display_runtime);
}


bool qPrefPlanner::displayTransitions() const
{
	return prefs.display_transitions;
}
void qPrefPlanner::setDisplayTransitions(bool value)
{
	if (value != prefs.display_transitions) {
		prefs.display_transitions = value;
		diskDisplayTransitions(true);
		emit displayTransitionsChanged(value);
	}
}
void qPrefPlanner::diskDisplayTransitions(bool doSync)
{
	LOADSYNC_BOOL("/display_transitions", display_transitions);
}


bool qPrefPlanner::displayVariations() const
{
	return prefs.display_variations;
}
void qPrefPlanner::setDisplayVariations(bool value)
{
	if (value != prefs.display_variations) {
		prefs.display_variations = value;
		diskDisplayVariations(true);
		emit displayVariationsChanged(value);
	}
}
void qPrefPlanner::diskDisplayVariations(bool doSync)
{
	LOADSYNC_BOOL("/display_variations", display_variations);
}


bool qPrefPlanner::doo2Breaks() const
{
	return prefs.doo2breaks;
}
void qPrefPlanner::setDoo2Breaks(bool value)
{
	if (value != prefs.doo2breaks) {
		prefs.doo2breaks = value;
		diskDoo2Breaks(true);
		emit doo2BreaksChanged(true);
	}
}
void qPrefPlanner::diskDoo2Breaks(bool doSync)
{
	LOADSYNC_BOOL("/doo2breaks", doo2breaks);
}


bool qPrefPlanner::dropStoneMode() const
{
	return prefs.drop_stone_mode;
}
void qPrefPlanner::setDropStoneMode(bool value)
{
	if (value != prefs.drop_stone_mode) {
		prefs.drop_stone_mode = value;
		diskDropStoneMode(true);
		emit dropStoneModeChanged(value);
	}
}
void qPrefPlanner::diskDropStoneMode(bool doSync)
{
	LOADSYNC_BOOL("/drop_stone_mode", drop_stone_mode);
}


bool qPrefPlanner::lastStop() const
{
	return prefs.last_stop;
}
void qPrefPlanner::setLastStop(bool value)
{
	if (value != prefs.last_stop) {
		prefs.last_stop = value;
		diskLastStop(true);
		emit lastStopChanged(value);
	}
}
void qPrefPlanner::diskLastStop(bool doSync)
{
	LOADSYNC_BOOL("/last_stop", last_stop);
}


int qPrefPlanner::minSwitchDuration() const
{
	return prefs.min_switch_duration;
}
void qPrefPlanner::setMinSwitchDuration(int value)
{
	if (value != prefs.min_switch_duration) {
		prefs.min_switch_duration = value;
		diskMinSwitchDuration(true);
		emit minSwitchDurationChanged(value);
	}
}
void qPrefPlanner::diskMinSwitchDuration(bool doSync)
{
	LOADSYNC_INT("/min_switch_duration", min_switch_duration);
}


int qPrefPlanner::problemSolvingTime() const
{
	return prefs.problemsolvingtime;
}
void qPrefPlanner::setProblemSolvingTime(int value)
{
	if (value != prefs.problemsolvingtime) {
		prefs.problemsolvingtime = value;
		diskProblemSolvingTime(true);
		emit problemSolvingTimeChanged(value);
	}
}
void qPrefPlanner::diskProblemSolvingTime(bool doSync)
{
	LOADSYNC_INT("/problemsolvingtime", problemsolvingtime);
}


int qPrefPlanner::reserveGas() const
{
	return prefs.reserve_gas;
}
void qPrefPlanner::setReserveGas(int value)
{
	if (value != prefs.reserve_gas) {
		prefs.reserve_gas = value;
		diskReserveGas(true);
		emit reserveGasChanged(value);
	}
}
void qPrefPlanner::diskReserveGas(bool doSync)
{
	LOADSYNC_INT("/reserve_gas", reserve_gas);
}



int qPrefPlanner::sacFactor() const
{
	return prefs.sacfactor;
}
void qPrefPlanner::setSacFactor(int value)
{
	if (value != prefs.sacfactor) {
		prefs.sacfactor = value;
		diskSacFactor(true);
		emit sacFactorChanged(value);
	}
}
void qPrefPlanner::diskSacFactor(bool doSync)
{
	LOADSYNC_INT("/sacfactor", sacfactor);
}


bool qPrefPlanner::safetyStop() const
{
	return prefs.safetystop;
}
void qPrefPlanner::setSafetyStop(bool value)
{
	if (value != prefs.safetystop) {
		prefs.safetystop = value;
		diskSafetyStop(true);
		emit safetyStopChanged(value);
	}
}
void qPrefPlanner::diskSafetyStop(bool doSync)
{
	LOADSYNC_BOOL("/safetystop", safetystop);
}


bool qPrefPlanner::switchAtRequiredStop() const
{
	return prefs.switch_at_req_stop;
}
void qPrefPlanner::setSwitchAtRequiredStop(bool value)
{
	if (value != prefs.switch_at_req_stop) {
		prefs.switch_at_req_stop = value;
		diskSwitchAtRequiredStop(true);
		emit switchAtRequiredStopChanged(value);
	}
}
void qPrefPlanner::diskSwitchAtRequiredStop(bool doSync)
{
	LOADSYNC_BOOL("/switch_at_req_stop", switch_at_req_stop);
}


bool qPrefPlanner::verbatimPlan() const
{
	return prefs.verbatim_plan;
}
void qPrefPlanner::setVerbatimPlan(bool value)
{
	if (value != prefs.verbatim_plan) {
		prefs.verbatim_plan = value;
		diskVerbatimPlan(true);
		emit verbatimPlanChanged(value);
	}
}
void qPrefPlanner::diskVerbatimPlan(bool doSync)
{
	LOADSYNC_BOOL("/verbatim_plan", verbatim_plan);
}
