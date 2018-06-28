// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPrefDivePlanner.h"

#include <QSettings>


qPrefDivePlanner *qPrefDivePlanner::m_instance = NULL;
qPrefDivePlanner *qPrefDivePlanner::instance()
{
	if (!m_instance)
		m_instance = new qPrefDivePlanner;
	return m_instance;
}



void qPrefDivePlanner::loadSync(bool doSync)
{
	diskAscratelast6m(doSync);
	diskAscratestops(doSync);
	diskAscrate50(doSync);
	diskAscrate75(doSync);
	diskBestmixend(doSync);
	diskBottompo2(doSync);
	diskBottomSac(doSync);
	diskDecoMode(doSync);
	diskDecopo2(doSync);
	diskDecoSac(doSync);
	diskDescrate(doSync);
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


int qPrefDivePlanner::ascratelast6m() const
{
	return prefs.ascratelast6m;
}
void qPrefDivePlanner::setAscratelast6m(int value)
{
	if (value != prefs.ascratelast6m) {
		prefs.ascratelast6m = value;
		diskAscratelast6m(true);
		emit ascratelast6mChanged(value);

	}
}
void qPrefDivePlanner::diskAscratelast6m(bool doSync)
{
	LOADSYNC_INT("/ascratelast6m", ascratelast6m);
}


int qPrefDivePlanner::ascratestops() const
{
	return prefs.ascratestops;
}
void qPrefDivePlanner::setAscratestops(int value)
{
	if (value != prefs.ascratestops) {
		prefs.ascratestops = value;
		diskAscratestops(true);
		emit ascratestopsChanged(value);
	}
}
void qPrefDivePlanner::diskAscratestops(bool doSync)
{
	LOADSYNC_INT("/ascratestops", ascratestops);
}


int qPrefDivePlanner::ascrate50() const
{
	return prefs.ascrate50;
}
void qPrefDivePlanner::setAscrate50(int value)
{
	if (value != prefs.ascrate50) {
		prefs.ascrate50 = value;
		diskAscrate50(true);
		emit ascrate50Changed(value);
	}
}
void qPrefDivePlanner::diskAscrate50(bool doSync)
{
	LOADSYNC_INT("/ascrate50", ascrate50);
}


int qPrefDivePlanner::ascrate75() const
{
	return prefs.ascrate75;
}
void qPrefDivePlanner::setAscrate75(int value)
{
	if (value != prefs.ascrate75) {
		prefs.ascrate75 = value;
		diskAscrate75(true);
		emit ascrate75Changed(value);
	}
}
void qPrefDivePlanner::diskAscrate75(bool doSync)
{
	LOADSYNC_INT("/ascrate75", ascrate75);
}


int qPrefDivePlanner::bestmixend() const
{
	return prefs.bestmixend.mm;
}
void qPrefDivePlanner::setBestmixend(int value)
{
	if (value != prefs.bestmixend.mm) {
		prefs.bestmixend.mm = value;
		diskBestmixend(true);
	}
}
void qPrefDivePlanner::diskBestmixend(bool doSync)
{
	LOADSYNC_INT("/bestmixend", bestmixend.mm);
}

int qPrefDivePlanner::bottompo2() const
{
	return prefs.bottompo2;
}
void qPrefDivePlanner::setBottompo2(int value)
{
	if (value != prefs.bottompo2) {
		prefs.bottompo2 = value;
		diskBottompo2(true);
		emit bottompo2Changed(value);
	}
}
void qPrefDivePlanner::diskBottompo2(bool doSync)
{
	LOADSYNC_INT("/bottompo2", bottompo2);
}


int qPrefDivePlanner::bottomSac() const
{
	return prefs.bottomsac;
}
void qPrefDivePlanner::setBottomSac(int value)
{
	if (value != prefs.bottomsac) {
		prefs.bottomsac = value;
		diskBottomSac(true);
	}
}
void qPrefDivePlanner::diskBottomSac(bool doSync)
{
	LOADSYNC_INT("/bottomsac", bottomsac);
}

deco_mode qPrefDivePlanner::decoMode() const
{
	return prefs.planner_deco_mode;
}
void qPrefDivePlanner::setDecoMode(deco_mode value)
{
	if (value != prefs.planner_deco_mode) {
		prefs.planner_deco_mode = value;
		diskDecoMode(true);
		emit decoModeChanged(value);

	}
}
void qPrefDivePlanner::diskDecoMode(bool doSync)
{
	LOADSYNC_ENUM("/deco_mode", deco_mode, planner_deco_mode);
}


int qPrefDivePlanner::decopo2() const
{
	return prefs.decopo2;
}
void qPrefDivePlanner::setDecopo2(int value)
{
	if (value != prefs.decopo2) {
		prefs.decopo2 = value;
		diskDecopo2(true);
		emit decopo2Changed(value);
	}
}
void qPrefDivePlanner::diskDecopo2(bool doSync)
{
	LOADSYNC_INT("/decopo2", decopo2);
}


int qPrefDivePlanner::decoSac() const
{
	return prefs.decosac;
}
void qPrefDivePlanner::setDecoSac(int value)
{
	if (value != prefs.decosac) {
		prefs.decosac = value;
		diskDecoSac(true);
		emit decoSacChanged(value);
	}
}
void qPrefDivePlanner::diskDecoSac(bool doSync)
{
	LOADSYNC_INT("/decosac", decosac);
}


int qPrefDivePlanner::descrate() const
{
	return prefs.descrate;
}
void qPrefDivePlanner::setDescrate(int value)
{
	if (value != prefs.descrate) {
		prefs.descrate = value;
		diskDescrate(true);
		emit descrateChanged(value);
	}
}
void qPrefDivePlanner::diskDescrate(bool doSync)
{
	LOADSYNC_INT("/descrate", descrate);
}


bool qPrefDivePlanner::displayDuration() const
{
	return prefs.display_duration;
}
void qPrefDivePlanner::setDisplayDuration(bool value)
{
	if (value != prefs.display_duration) {
		prefs.display_duration = value;
		diskDisplayDuration(true);
		emit displayDurationChanged(value);
	}
}
void qPrefDivePlanner::diskDisplayDuration(bool doSync)
{
	LOADSYNC_BOOL("/display_duration", display_duration);
}


bool qPrefDivePlanner::displayRuntime() const
{
	return prefs.display_runtime;
}
void qPrefDivePlanner::setDisplayRuntime(bool value)
{
	if (value != prefs.display_runtime) {
		prefs.display_runtime = value;
		diskDisplayRuntime(true);
		emit displayRuntimeChanged(value);
	}
}
void qPrefDivePlanner::diskDisplayRuntime(bool doSync)
{
	LOADSYNC_BOOL("/display_runtime", display_runtime);
}


bool qPrefDivePlanner::displayTransitions() const
{
	return prefs.display_transitions;
}
void qPrefDivePlanner::setDisplayTransitions(bool value)
{
	if (value != prefs.display_transitions) {
		prefs.display_transitions = value;
		diskDisplayTransitions(true);
		emit displayTransitionsChanged(value);
	}
}
void qPrefDivePlanner::diskDisplayTransitions(bool doSync)
{
	LOADSYNC_BOOL("/display_transitions", display_transitions);
}


bool qPrefDivePlanner::displayVariations() const
{
	return prefs.display_variations;
}
void qPrefDivePlanner::setDisplayVariations(bool value)
{
	if (value != prefs.display_variations) {
		prefs.display_variations = value;
		diskDisplayVariations(true);
		emit displayVariationsChanged(value);
	}
}
void qPrefDivePlanner::diskDisplayVariations(bool doSync)
{
	LOADSYNC_BOOL("/display_variations", display_variations);
}


bool qPrefDivePlanner::doo2breaks() const
{
	return prefs.doo2breaks;
}
void qPrefDivePlanner::setDoo2breaks(bool value)
{
	if (value != prefs.doo2breaks) {
		prefs.doo2breaks = value;
		diskDoo2Breaks(true);
		emit doo2breaksChanged(true);
	}
}
void qPrefDivePlanner::diskDoo2Breaks(bool doSync)
{
	LOADSYNC_BOOL("/doo2breaks", doo2breaks);
}


bool qPrefDivePlanner::dropStoneMode() const
{
	return prefs.drop_stone_mode;
}
void qPrefDivePlanner::setDropStoneMode(bool value)
{
	if (value != prefs.drop_stone_mode) {
		prefs.drop_stone_mode = value;
		diskDropStoneMode(true);
		emit dropStoneModeChanged(value);
	}
}
void qPrefDivePlanner::diskDropStoneMode(bool doSync)
{
	LOADSYNC_BOOL("/drop_stone_mode", drop_stone_mode);
}


bool qPrefDivePlanner::lastStop() const
{
	return prefs.last_stop;
}
void qPrefDivePlanner::setLastStop(bool value)
{
	if (value != prefs.last_stop) {
		prefs.last_stop = value;
		diskLastStop(true);
		emit lastStopChanged(value);
	}
}
void qPrefDivePlanner::diskLastStop(bool doSync)
{
	LOADSYNC_BOOL("/last_stop", last_stop);
}


int qPrefDivePlanner::minSwitchDuration() const
{
	return prefs.min_switch_duration;
}
void qPrefDivePlanner::setMinSwitchDuration(int value)
{
	if (value != prefs.min_switch_duration) {
		prefs.min_switch_duration = value;
		diskMinSwitchDuration(true);
		emit minSwitchDurationChanged(value);
	}
}
void qPrefDivePlanner::diskMinSwitchDuration(bool doSync)
{
	LOADSYNC_INT("/min_switch_duration", min_switch_duration);
}


int qPrefDivePlanner::problemSolvingTime() const
{
	return prefs.problemsolvingtime;
}
void qPrefDivePlanner::setProblemSolvingTime(int value)
{
	if (value != prefs.problemsolvingtime) {
		prefs.problemsolvingtime = value;
		diskProblemSolvingTime(true);
		emit problemSolvingTimeChanged(value);
	}
}
void qPrefDivePlanner::diskProblemSolvingTime(bool doSync)
{
	LOADSYNC_INT("/problemsolvingtime", problemsolvingtime);
}


int qPrefDivePlanner::reserveGas() const
{
	return prefs.reserve_gas;
}
void qPrefDivePlanner::setReserveGas(int value)
{
	if (value != prefs.reserve_gas) {
		prefs.reserve_gas = value;
		diskReserveGas(true);
		emit reserveGasChanged(value);
	}
}
void qPrefDivePlanner::diskReserveGas(bool doSync)
{
	LOADSYNC_INT("/reserve_gas", reserve_gas);
}



int qPrefDivePlanner::sacFactor() const
{
	return prefs.sacfactor;
}
void qPrefDivePlanner::setSacFactor(int value)
{
	if (value != prefs.sacfactor) {
		prefs.sacfactor = value;
		diskSacFactor(true);
		emit sacFactorChanged(value);
	}
}
void qPrefDivePlanner::diskSacFactor(bool doSync)
{
	LOADSYNC_INT("/sacfactor", sacfactor);
}


bool qPrefDivePlanner::safetyStop() const
{
	return prefs.safetystop;
}
void qPrefDivePlanner::setSafetyStop(bool value)
{
	if (value != prefs.safetystop) {
		prefs.safetystop = value;
		diskSafetyStop(true);
		emit safetyStopChanged(value);
	}
}
void qPrefDivePlanner::diskSafetyStop(bool doSync)
{
	LOADSYNC_BOOL("/safetystop", safetystop);
}


bool qPrefDivePlanner::switchAtRequiredStop() const
{
	return prefs.switch_at_req_stop;
}
void qPrefDivePlanner::setSwitchAtRequiredStop(bool value)
{
	if (value != prefs.switch_at_req_stop) {
		prefs.switch_at_req_stop = value;
		diskSwitchAtRequiredStop(true);
		emit switchAtRequiredStopChanged(value);
	}
}
void qPrefDivePlanner::diskSwitchAtRequiredStop(bool doSync)
{
	LOADSYNC_BOOL("/switch_at_req_stop", switch_at_req_stop);
}


bool qPrefDivePlanner::verbatimPlan() const
{
	return prefs.verbatim_plan;
}
void qPrefDivePlanner::setVerbatimPlan(bool value)
{
	if (value != prefs.verbatim_plan) {
		prefs.verbatim_plan = value;
		diskVerbatimPlan(true);
		emit verbatimPlanChanged(value);
	}
}
void qPrefDivePlanner::diskVerbatimPlan(bool doSync)
{
	LOADSYNC_BOOL("/verbatim_plan", verbatim_plan);
}
