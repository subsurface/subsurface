// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>
#include <QNetworkProxy>

#include "core/qthelper.h"

UpdateManagerSettings::UpdateManagerSettings(QObject *parent) : QObject(parent)
{

}

bool UpdateManagerSettings::dontCheckForUpdates() const
{
	return prefs.update_manager.dont_check_for_updates;
}

bool UpdateManagerSettings::dontCheckExists() const
{
	return prefs.update_manager.dont_check_exists;
}

QString UpdateManagerSettings::lastVersionUsed() const
{
	return prefs.update_manager.last_version_used;
}

QDate UpdateManagerSettings::nextCheck() const
{
	return QDate::fromString(QString(prefs.update_manager.next_check), "dd/MM/yyyy");
}

void UpdateManagerSettings::setDontCheckForUpdates(bool value)
{
	if (value == prefs.update_manager.dont_check_for_updates)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("DontCheckForUpdates", value);
	prefs.update_manager.dont_check_for_updates = value;
	prefs.update_manager.dont_check_exists = true;
	emit dontCheckForUpdatesChanged(value);
}

void UpdateManagerSettings::setLastVersionUsed(const QString& value)
{
	if (value == prefs.update_manager.last_version_used)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("LastVersionUsed", value);
	free((void *)prefs.update_manager.last_version_used);
	prefs.update_manager.last_version_used = copy_qstring(value);
	emit lastVersionUsedChanged(value);
}

void UpdateManagerSettings::setNextCheck(const QDate& date)
{
	if (date.toString() == prefs.update_manager.next_check)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("NextCheck", date);
	free((void *)prefs.update_manager.next_check);
	prefs.update_manager.next_check = copy_qstring(date.toString("dd/MM/yyyy"));
	emit nextCheckChanged(date);
}


PartialPressureGasSettings::PartialPressureGasSettings(QObject* parent):
	QObject(parent)
{

}

bool PartialPressureGasSettings::showPo2() const
{
	return prefs.pp_graphs.po2;
}

bool PartialPressureGasSettings::showPn2() const
{
	return prefs.pp_graphs.pn2;
}

bool PartialPressureGasSettings::showPhe() const
{
	return prefs.pp_graphs.phe;
}

double PartialPressureGasSettings::po2ThresholdMin() const
{
	return prefs.pp_graphs.po2_threshold_min;
}

double PartialPressureGasSettings::po2ThresholdMax() const
{
	return prefs.pp_graphs.po2_threshold_max;
}


double PartialPressureGasSettings::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}

double PartialPressureGasSettings::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}

void PartialPressureGasSettings::setShowPo2(bool value)
{
	if (value == prefs.pp_graphs.po2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2graph", value);
	prefs.pp_graphs.po2 = value;
	emit showPo2Changed(value);
}

void PartialPressureGasSettings::setShowPn2(bool value)
{
	if (value == prefs.pp_graphs.pn2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("pn2graph", value);
	prefs.pp_graphs.pn2 = value;
	emit showPn2Changed(value);
}

void PartialPressureGasSettings::setShowPhe(bool value)
{
	if (value == prefs.pp_graphs.phe)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("phegraph", value);
	prefs.pp_graphs.phe = value;
	emit showPheChanged(value);
}

void PartialPressureGasSettings::setPo2ThresholdMin(double value)
{
	if (value == prefs.pp_graphs.po2_threshold_min)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2thresholdmin", value);
	prefs.pp_graphs.po2_threshold_min = value;
	emit po2ThresholdMinChanged(value);
}

void PartialPressureGasSettings::setPo2ThresholdMax(double value)
{
	if (value == prefs.pp_graphs.po2_threshold_max)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2thresholdmax", value);
	prefs.pp_graphs.po2_threshold_max = value;
	emit po2ThresholdMaxChanged(value);
}

void PartialPressureGasSettings::setPn2Threshold(double value)
{
	if (value == prefs.pp_graphs.pn2_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("pn2threshold", value);
	prefs.pp_graphs.pn2_threshold = value;
	emit pn2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPheThreshold(double value)
{
	if (value == prefs.pp_graphs.phe_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("phethreshold", value);
	prefs.pp_graphs.phe_threshold = value;
	emit pheThresholdChanged(value);
}


TechnicalDetailsSettings::TechnicalDetailsSettings(QObject* parent): QObject(parent)
{

}

deco_mode TechnicalDetailsSettings::deco() const
{

	return prefs.display_deco_mode;
}

void TechnicalDetailsSettings::setDecoMode(deco_mode d)
{
	if (prefs.display_deco_mode == d)
		return;

	prefs.display_deco_mode = d;
	QSettings s;
	s.beginGroup(group);
	s.setValue("display_deco_mode", d);
	emit decoModeChanged(d);
}

double TechnicalDetailsSettings:: modpO2() const
{
	return prefs.modpO2;
}

bool TechnicalDetailsSettings::ead() const
{
	return prefs.ead;
}

bool TechnicalDetailsSettings::dcceiling() const
{
	return prefs.dcceiling;
}

bool TechnicalDetailsSettings::redceiling() const
{
	return prefs.redceiling;
}

bool TechnicalDetailsSettings::calcceiling() const
{
	return prefs.calcceiling;
}

bool TechnicalDetailsSettings::calcceiling3m() const
{
	return prefs.calcceiling3m;
}

bool TechnicalDetailsSettings::calcalltissues() const
{
	return prefs.calcalltissues;
}

bool TechnicalDetailsSettings::calcndltts() const
{
	return prefs.calcndltts;
}

bool TechnicalDetailsSettings::buehlmann() const
{
	return prefs.planner_deco_mode == BUEHLMANN;
}

int TechnicalDetailsSettings::gflow() const
{
	return prefs.gflow;
}

int TechnicalDetailsSettings::gfhigh() const
{
	return prefs.gfhigh;
}

short TechnicalDetailsSettings::vpmbConservatism() const
{
	return prefs.vpmb_conservatism;
}

bool TechnicalDetailsSettings::hrgraph() const
{
	return prefs.hrgraph;
}

bool TechnicalDetailsSettings::tankBar() const
{
	return prefs.tankbar;
}

bool TechnicalDetailsSettings::percentageGraph() const
{
	return prefs.percentagegraph;
}

bool TechnicalDetailsSettings::rulerGraph() const
{
	return prefs.rulergraph;
}

bool TechnicalDetailsSettings::showSCROCpO2() const
{
	return prefs.show_scr_ocpo2;
}

bool TechnicalDetailsSettings::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}

bool TechnicalDetailsSettings::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}

bool TechnicalDetailsSettings::zoomedPlot() const
{
	return prefs.zoomed_plot;
}

bool TechnicalDetailsSettings::showSac() const
{
	return prefs.show_sac;
}

bool TechnicalDetailsSettings::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}

bool TechnicalDetailsSettings::showAverageDepth() const
{
	return prefs.show_average_depth;
}

bool TechnicalDetailsSettings::showIcd() const
{
	return prefs.show_icd;
}

bool TechnicalDetailsSettings::mod() const
{
	return prefs.mod;
}

bool TechnicalDetailsSettings::showPicturesInProfile() const
{
	return prefs.show_pictures_in_profile;
}

void TechnicalDetailsSettings::setModpO2(double value)
{
	if (value == prefs.modpO2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("modpO2", value);
	prefs.modpO2 = value;
	emit modpO2Changed(value);
}

void TechnicalDetailsSettings::setShowPicturesInProfile(bool value)
{
	if (value == prefs.show_pictures_in_profile)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_pictures_in_profile", value);
	prefs.show_pictures_in_profile = value;
	emit showPicturesInProfileChanged(value);
}

void TechnicalDetailsSettings::setEad(bool value)
{
	if (value == prefs.ead)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ead", value);
	prefs.ead = value;
	emit eadChanged(value);
}

void TechnicalDetailsSettings::setMod(bool value)
{
	if (value == prefs.mod)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("mod", value);
	prefs.mod = value;
	emit modChanged(value);
}

void TechnicalDetailsSettings::setDCceiling(bool value)
{
	if (value == prefs.dcceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dcceiling", value);
	prefs.dcceiling = value;
	emit dcceilingChanged(value);
}

void TechnicalDetailsSettings::setRedceiling(bool value)
{
	if (value == prefs.redceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("redceiling", value);
	prefs.redceiling = value;
	emit redceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling(bool value)
{
	if (value == prefs.calcceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcceiling", value);
	prefs.calcceiling = value;
	emit calcceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling3m(bool value)
{
	if (value == prefs.calcceiling3m)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcceiling3m", value);
	prefs.calcceiling3m = value;
	emit calcceiling3mChanged(value);
}

void TechnicalDetailsSettings::setCalcalltissues(bool value)
{
	if (value == prefs.calcalltissues)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcalltissues", value);
	prefs.calcalltissues = value;
	emit calcalltissuesChanged(value);
}

void TechnicalDetailsSettings::setCalcndltts(bool value)
{
	if (value == prefs.calcndltts)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcndltts", value);
	prefs.calcndltts = value;
	emit calcndlttsChanged(value);
}

void TechnicalDetailsSettings::setBuehlmann(bool value)
{
	if (value == (prefs.planner_deco_mode == BUEHLMANN))
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("buehlmann", value);
	prefs.planner_deco_mode = value ? BUEHLMANN : VPMB;
	emit buehlmannChanged(value);
}

void TechnicalDetailsSettings::setGflow(int value)
{
	if (value == prefs.gflow)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("gflow", value);
	prefs.gflow = value;
	set_gf(prefs.gflow, prefs.gfhigh);
	emit gflowChanged(value);
}

void TechnicalDetailsSettings::setGfhigh(int value)
{
	if (value == prefs.gfhigh)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("gfhigh", value);
	prefs.gfhigh = value;
	set_gf(prefs.gflow, prefs.gfhigh);
	emit gfhighChanged(value);
}

void TechnicalDetailsSettings::setVpmbConservatism(short value)
{
	if (value == prefs.vpmb_conservatism)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("vpmb_conservatism", value);
	prefs.vpmb_conservatism = value;
	set_vpmb_conservatism(value);
	emit vpmbConservatismChanged(value);
}

void TechnicalDetailsSettings::setHRgraph(bool value)
{
	if (value == prefs.hrgraph)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("hrgraph", value);
	prefs.hrgraph = value;
	emit hrgraphChanged(value);
}

void TechnicalDetailsSettings::setTankBar(bool value)
{
	if (value == prefs.tankbar)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("tankbar", value);
	prefs.tankbar = value;
	emit tankBarChanged(value);
}

void TechnicalDetailsSettings::setPercentageGraph(bool value)
{
	if (value == prefs.percentagegraph)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("percentagegraph", value);
	prefs.percentagegraph = value;
	emit percentageGraphChanged(value);
}

void TechnicalDetailsSettings::setRulerGraph(bool value)
{
	if (value == prefs.rulergraph)
		return;
	/* TODO: search for the QSettings of the RulerBar */
	QSettings s;
	s.beginGroup(group);
	s.setValue("RulerBar", value);
	prefs.rulergraph = value;
	emit rulerGraphChanged(value);
}

void TechnicalDetailsSettings::setShowCCRSetpoint(bool value)
{
	if (value == prefs.show_ccr_setpoint)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_ccr_setpoint", value);
	prefs.show_ccr_setpoint = value;
	emit showCCRSetpointChanged(value);
}

void TechnicalDetailsSettings::setShowSCROCpO2(bool value)
{
	if (value == prefs.show_scr_ocpo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_scr_ocpo2", value);
	prefs.show_scr_ocpo2 = value;
	emit showSCROCpO2Changed(value);
}

void TechnicalDetailsSettings::setShowCCRSensors(bool value)
{
	if (value == prefs.show_ccr_sensors)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_ccr_sensors", value);
	prefs.show_ccr_sensors = value;
	emit showCCRSensorsChanged(value);
}

void TechnicalDetailsSettings::setZoomedPlot(bool value)
{
	if (value == prefs.zoomed_plot)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("zoomed_plot", value);
	prefs.zoomed_plot = value;
	emit zoomedPlotChanged(value);
}

void TechnicalDetailsSettings::setShowSac(bool value)
{
	if (value == prefs.show_sac)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_sac", value);
	prefs.show_sac = value;
	emit showSacChanged(value);
}

void TechnicalDetailsSettings::setDisplayUnusedTanks(bool value)
{
	if (value == prefs.display_unused_tanks)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("display_unused_tanks", value);
	prefs.display_unused_tanks = value;
	emit displayUnusedTanksChanged(value);
}

void TechnicalDetailsSettings::setShowAverageDepth(bool value)
{
	if (value == prefs.show_average_depth)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_average_depth", value);
	prefs.show_average_depth = value;
	emit showAverageDepthChanged(value);
}

void TechnicalDetailsSettings::setShowIcd(bool value)
{
	if (value == prefs.show_icd)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_icd", value);
	prefs.show_icd = value;
	emit showIcdChanged(value);
}

ProxySettings::ProxySettings(QObject *parent) :
	QObject(parent)
{
}

int ProxySettings::type() const
{
	return prefs.proxy_type;
}

QString ProxySettings::host() const
{
	return prefs.proxy_host;
}

int ProxySettings::port() const
{
	return prefs.proxy_port;
}

bool ProxySettings::auth() const
{
	return prefs.proxy_auth;
}

QString ProxySettings::user() const
{
	return prefs.proxy_user;
}

QString ProxySettings::pass() const
{
	return prefs.proxy_pass;
}

void ProxySettings::setType(int value)
{
	if (value == prefs.proxy_type)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_type", value);
	prefs.proxy_type = value;
	emit typeChanged(value);
}

void ProxySettings::setHost(const QString& value)
{
	if (value == prefs.proxy_host)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_host", value);
	free((void *)prefs.proxy_host);
	prefs.proxy_host = copy_qstring(value);
	emit hostChanged(value);
}

void ProxySettings::setPort(int value)
{
	if (value == prefs.proxy_port)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_port", value);
	prefs.proxy_port = value;
	emit portChanged(value);
}

void ProxySettings::setAuth(bool value)
{
	if (value == prefs.proxy_auth)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_auth", value);
	prefs.proxy_auth = value;
	emit authChanged(value);
}

void ProxySettings::setUser(const QString& value)
{
	if (value == prefs.proxy_user)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_user", value);
	free((void *)prefs.proxy_user);
	prefs.proxy_user = copy_qstring(value);
	emit userChanged(value);
}

void ProxySettings::setPass(const QString& value)
{
	if (value == prefs.proxy_pass)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("proxy_pass", value);
	free((void *)prefs.proxy_pass);
	prefs.proxy_pass = copy_qstring(value);
	emit passChanged(value);
}

DivePlannerSettings::DivePlannerSettings(QObject *parent) :
	QObject(parent)
{
}

bool DivePlannerSettings::lastStop() const
{
	return prefs.last_stop;
}

bool DivePlannerSettings::verbatimPlan() const
{
	return prefs.verbatim_plan;
}

bool DivePlannerSettings::displayRuntime() const
{
	return prefs.display_runtime;
}

bool DivePlannerSettings::displayDuration() const
{
	return prefs.display_duration;
}

bool DivePlannerSettings::displayTransitions() const
{
	return prefs.display_transitions;
}

bool DivePlannerSettings::displayVariations() const
{
	return prefs.display_variations;
}

bool DivePlannerSettings::doo2breaks() const
{
	return prefs.doo2breaks;
}

bool DivePlannerSettings::dropStoneMode() const
{
	return prefs.drop_stone_mode;
}

bool DivePlannerSettings::safetyStop() const
{
	return prefs.safetystop;
}

bool DivePlannerSettings::switchAtRequiredStop() const
{
	return prefs.switch_at_req_stop;
}

int DivePlannerSettings::ascrate75() const
{
	return prefs.ascrate75;
}

int DivePlannerSettings::ascrate50() const
{
	return prefs.ascrate50;
}

int DivePlannerSettings::ascratestops() const
{
	return prefs.ascratestops;
}

int DivePlannerSettings::ascratelast6m() const
{
	return prefs.ascratelast6m;
}

int DivePlannerSettings::descrate() const
{
	return prefs.descrate;
}

int DivePlannerSettings::sacfactor() const
{
	return prefs.sacfactor;
}

int DivePlannerSettings::problemsolvingtime() const
{
	return prefs.problemsolvingtime;
}

int DivePlannerSettings::bottompo2() const
{
	return prefs.bottompo2;
}

int DivePlannerSettings::decopo2() const
{
	return prefs.decopo2;
}

int DivePlannerSettings::bestmixend() const
{
	return prefs.bestmixend.mm;
}

int DivePlannerSettings::reserveGas() const
{
	return prefs.reserve_gas;
}

int DivePlannerSettings::minSwitchDuration() const
{
	return prefs.min_switch_duration;
}

int DivePlannerSettings::bottomSac() const
{
	return prefs.bottomsac;
}

int DivePlannerSettings::decoSac() const
{
	return prefs.decosac;
}

deco_mode DivePlannerSettings::decoMode() const
{
	return prefs.planner_deco_mode;
}

void DivePlannerSettings::setLastStop(bool value)
{
	if (value == prefs.last_stop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("last_stop", value);
	prefs.last_stop = value;
	emit lastStopChanged(value);
}

void DivePlannerSettings::setVerbatimPlan(bool value)
{
	if (value == prefs.verbatim_plan)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("verbatim_plan", value);
	prefs.verbatim_plan = value;
	emit verbatimPlanChanged(value);
}

void DivePlannerSettings::setDisplayRuntime(bool value)
{
	if (value == prefs.display_runtime)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_runtime", value);
	prefs.display_runtime = value;
	emit displayRuntimeChanged(value);
}

void DivePlannerSettings::setDisplayDuration(bool value)
{
	if (value == prefs.display_duration)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_duration", value);
	prefs.display_duration = value;
	emit displayDurationChanged(value);
}

void DivePlannerSettings::setDisplayTransitions(bool value)
{
	if (value == prefs.display_transitions)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_transitions", value);
	prefs.display_transitions = value;
	emit displayTransitionsChanged(value);
}

void DivePlannerSettings::setDisplayVariations(bool value)
{
	if (value == prefs.display_variations)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("display_variations", value);
	prefs.display_variations = value;
	emit displayVariationsChanged(value);
}

void DivePlannerSettings::setDoo2breaks(bool value)
{
	if (value == prefs.doo2breaks)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("doo2breaks", value);
	prefs.doo2breaks = value;
	emit doo2breaksChanged(value);
}

void DivePlannerSettings::setDropStoneMode(bool value)
{
	if (value == prefs.drop_stone_mode)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("drop_stone_mode", value);
	prefs.drop_stone_mode = value;
	emit dropStoneModeChanged(value);
}

void DivePlannerSettings::setSafetyStop(bool value)
{
	if (value == prefs.safetystop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("safetystop", value);
	prefs.safetystop = value;
	emit safetyStopChanged(value);
}

void DivePlannerSettings::setSwitchAtRequiredStop(bool value)
{
	if (value == prefs.switch_at_req_stop)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("switch_at_req_stop", value);
	prefs.switch_at_req_stop = value;
	emit switchAtRequiredStopChanged(value);
}

void DivePlannerSettings::setAscrate75(int value)
{
	if (value == prefs.ascrate75)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("ascrate75", value);
	prefs.ascrate75 = value;
	emit ascrate75Changed(value);
}

void DivePlannerSettings::setAscrate50(int value)
{
	if (value == prefs.ascrate50)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ascrate50", value);
	prefs.ascrate50 = value;
	emit ascrate50Changed(value);
}

void DivePlannerSettings::setAscratestops(int value)
{
	if (value == prefs.ascratestops)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("ascratestops", value);
	prefs.ascratestops = value;
	emit ascratestopsChanged(value);
}

void DivePlannerSettings::setAscratelast6m(int value)
{
	if (value == prefs.ascratelast6m)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ascratelast6m", value);
	prefs.ascratelast6m = value;
	emit ascratelast6mChanged(value);
}

void DivePlannerSettings::setDescrate(int value)
{
	if (value == prefs.descrate)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("descrate", value);
	prefs.descrate = value;
	emit descrateChanged(value);
}

void DivePlannerSettings::setSacFactor(int value)
{
	if (value == prefs.sacfactor)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("sacfactor", value);
	prefs.sacfactor = value;
	emit sacFactorChanged(value);
}

void DivePlannerSettings::setProblemSolvingTime(int value)
{
	if (value == prefs.problemsolvingtime)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("problemsolvingtime", value);
	prefs.problemsolvingtime = value;
	emit problemSolvingTimeChanged(value);
}

void DivePlannerSettings::setBottompo2(int value)
{
	if (value == prefs.bottompo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bottompo2", value);
	prefs.bottompo2 = value;
	emit bottompo2Changed(value);
}

void DivePlannerSettings::setDecopo2(int value)
{
	if (value == prefs.decopo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("decopo2", value);
	prefs.decopo2 = value;
	emit decopo2Changed(value);
}

void DivePlannerSettings::setBestmixend(int value)
{
	if (value == prefs.bestmixend.mm)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bestmixend", value);
	prefs.bestmixend.mm = value;
	emit bestmixendChanged(value);
}

void DivePlannerSettings::setReserveGas(int value)
{
	if (value == prefs.reserve_gas)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("reserve_gas", value);
	prefs.reserve_gas = value;
	emit reserveGasChanged(value);
}

void DivePlannerSettings::setMinSwitchDuration(int value)
{
	if (value == prefs.min_switch_duration)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("min_switch_duration", value);
	prefs.min_switch_duration = value;
	emit minSwitchDurationChanged(value);
}

void DivePlannerSettings::setBottomSac(int value)
{
	if (value == prefs.bottomsac)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("bottomsac", value);
	prefs.bottomsac = value;
	emit bottomSacChanged(value);
}

void DivePlannerSettings::setDecoSac(int value)
{
	if (value == prefs.decosac)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("decosac", value);
	prefs.decosac = value;
	emit decoSacChanged(value);
}

void DivePlannerSettings::setDecoMode(deco_mode value)
{
	if (value == prefs.planner_deco_mode)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("deco_mode", value);
	prefs.planner_deco_mode = value;
	emit decoModeChanged(value);
}

UnitsSettings::UnitsSettings(QObject *parent) :
	QObject(parent)
{

}

int UnitsSettings::length() const
{
	return prefs.units.length;
}

int UnitsSettings::pressure() const
{
	return prefs.units.pressure;
}

int UnitsSettings::volume() const
{
	return prefs.units.volume;
}

int UnitsSettings::temperature() const
{
	return prefs.units.temperature;
}

int UnitsSettings::weight() const
{
	return prefs.units.weight;
}

int UnitsSettings::verticalSpeedTime() const
{
	return prefs.units.vertical_speed_time;
}

int UnitsSettings::durationUnits() const
{
	return prefs.units.duration_units;
}

bool UnitsSettings::showUnitsTable() const
{
	return prefs.units.show_units_table;
}

QString UnitsSettings::unitSystem() const
{
	return prefs.unit_system == METRIC ? QStringLiteral("metric")
			: prefs.unit_system == IMPERIAL ? QStringLiteral("imperial")
			: QStringLiteral("personalized");
}

bool UnitsSettings::coordinatesTraditional() const
{
	return prefs.coordinates_traditional;
}

void UnitsSettings::setLength(int value)
{
	if (value == prefs.units.length)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("length", value);
	prefs.units.length = (units::LENGTH) value;
	emit lengthChanged(value);
}

void UnitsSettings::setPressure(int value)
{
	if (value == prefs.units.pressure)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("pressure", value);
	prefs.units.pressure = (units::PRESSURE) value;
	emit pressureChanged(value);
}

void UnitsSettings::setVolume(int value)
{
	if (value == prefs.units.volume)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("volume", value);
	prefs.units.volume = (units::VOLUME) value;
	emit volumeChanged(value);
}

void UnitsSettings::setTemperature(int value)
{
	if (value == prefs.units.temperature)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("temperature", value);
	prefs.units.temperature = (units::TEMPERATURE) value;
	emit temperatureChanged(value);
}

void UnitsSettings::setWeight(int value)
{
	if (value == prefs.units.weight)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("weight", value);
	prefs.units.weight = (units::WEIGHT) value;
	emit weightChanged(value);
}

void UnitsSettings::setVerticalSpeedTime(int value)
{
	if (value == prefs.units.vertical_speed_time)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("vertical_speed_time", value);
	prefs.units.vertical_speed_time = (units::TIME) value;
	emit verticalSpeedTimeChanged(value);
}

void UnitsSettings::setDurationUnits(int value)
{
	if (value == prefs.units.duration_units)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("duration_units", value);
	prefs.units.duration_units = (units::DURATION) value;
	emit durationUnitChanged(value);
}

void UnitsSettings::setShowUnitsTable(bool value)
{
	if (value == prefs.units.show_units_table)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_units_table", value);
	prefs.units.show_units_table = value;
	emit showUnitsTableChanged(value);
}

void UnitsSettings::setCoordinatesTraditional(bool value)
{
	if (value == prefs.coordinates_traditional)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("coordinates", value);
	prefs.coordinates_traditional = value;
	emit coordinatesTraditionalChanged(value);
}

void UnitsSettings::setUnitSystem(const QString& value)
{
	short int v = value == QStringLiteral("metric") ? METRIC
		      : value == QStringLiteral("imperial")? IMPERIAL
		      : PERSONALIZE;

	if (v == prefs.unit_system)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("unit_system", value);

	if (value == QStringLiteral("metric")) {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (value == QStringLiteral("imperial")) {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
	}

	emit unitSystemChanged(value);
	// TODO: emit the other values here?
}

SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new TechnicalDetailsSettings(this)),
	pp_gas(new PartialPressureGasSettings(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new qPrefGeocoding(this)),
	proxy(new ProxySettings(this)),
	cloud_storage(new qPrefCloudStorage(this)),
	planner_settings(new DivePlannerSettings(this)),
	unit_settings(new UnitsSettings(this)),
	general_settings(new qPrefGeneral(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new qPrefLanguage(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new qPrefLocationService(this)),
	update_manager_settings(new UpdateManagerSettings(this)),
	dive_computer_settings(new qPrefDiveComputer(this))
{
}

void SettingsObjectWrapper::load()
{
	QSettings s;
	QVariant v;

	uiLanguage(NULL);
	s.beginGroup("Units");
	if (s.value("unit_system").toString() == "metric") {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (s.value("unit_system").toString() == "imperial") {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
		GET_UNIT("length", length, units::FEET, units::METERS);
		GET_UNIT("pressure", pressure, units::PSI, units::BAR);
		GET_UNIT("volume", volume, units::CUFT, units::LITER);
		GET_UNIT("temperature", temperature, units::FAHRENHEIT, units::CELSIUS);
		GET_UNIT("weight", weight, units::LBS, units::KG);
	}
	GET_UNIT("vertical_speed_time", vertical_speed_time, units::MINUTES, units::SECONDS);
	GET_UNIT3("duration_units", duration_units, units::MIXED, units::ALWAYS_HOURS, units::DURATION);
	GET_UNIT_BOOL("show_units_table", show_units_table);
	GET_BOOL("coordinates", coordinates_traditional);
	s.endGroup();
	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2thresholdmin", pp_graphs.po2_threshold_min);
	GET_DOUBLE("po2thresholdmax", pp_graphs.po2_threshold_max);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modpO2", modpO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", redceiling);
	GET_BOOL("dcceiling", dcceiling);
	GET_BOOL("calcceiling", calcceiling);
	GET_BOOL("calcceiling3m", calcceiling3m);
	GET_BOOL("calcndltts", calcndltts);
	GET_BOOL("calcalltissues", calcalltissues);
	GET_BOOL("hrgraph", hrgraph);
	GET_BOOL("tankbar", tankbar);
	GET_BOOL("RulerBar", rulergraph);
	GET_BOOL("percentagegraph", percentagegraph);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_INT("vpmb_conservatism", vpmb_conservatism);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	GET_BOOL("show_ccr_setpoint",show_ccr_setpoint);
	GET_BOOL("show_ccr_sensors",show_ccr_sensors);
	GET_BOOL("show_scr_ocpo2",show_scr_ocpo2);
	GET_BOOL("zoomed_plot", zoomed_plot);
	set_gf(prefs.gflow, prefs.gfhigh);
	set_vpmb_conservatism(prefs.vpmb_conservatism);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	GET_BOOL("show_average_depth", show_average_depth);
	GET_BOOL("show_icd", show_icd);
	GET_BOOL("show_pictures_in_profile", show_pictures_in_profile);
	prefs.display_deco_mode =  (deco_mode) s.value("display_deco_mode").toInt();
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_INT("default_file_behavior", default_file_behavior);
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	GET_TXT("default_cylinder", default_cylinder);
	GET_BOOL("use_default_file", use_default_file);
	GET_INT("defaultsetpoint", defaultsetpoint);
	GET_INT("o2consumption", o2consumption);
	GET_INT("pscr_ratio", pscr_ratio);
	GET_BOOL("auto_recalculate_thumbnails", auto_recalculate_thumbnails);
	s.endGroup();

	s.beginGroup("Display");
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = s.value("divelist_font", prefs.divelist_font).value<QFont>();
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}
	prefs.font_size = s.value("font_size", prefs.font_size).toFloat();
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(qPrintable(fontName))) {
		defaultFont = QFont(prefs.divelist_font);
	} else {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = copy_qstring(fontName);
	}
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
	GET_BOOL("displayinvalid", display_invalid_dives);
	s.endGroup();

	s.beginGroup("Animations");
	GET_INT("animation_speed", animation_speed);
	s.endGroup();

	s.beginGroup("Network");
	GET_INT_DEF("proxy_type", proxy_type, QNetworkProxy::DefaultProxy);
	GET_TXT("proxy_host", proxy_host);
	GET_INT("proxy_port", proxy_port);
	GET_BOOL("proxy_auth", proxy_auth);
	GET_TXT("proxy_user", proxy_user);
	GET_TXT("proxy_pass", proxy_pass);
	s.endGroup();

	s.beginGroup("CloudStorage");
	GET_TXT("email", cloud_storage_email);
	GET_TXT("email_encoded", cloud_storage_email_encoded);
#ifndef SUBSURFACE_MOBILE
	GET_BOOL("save_password_local", save_password_local);
#else
	// always save the password in Subsurface-mobile
	prefs.save_password_local = true;
#endif
	if (prefs.save_password_local) { // GET_TEXT macro is not a single statement
		GET_TXT("password", cloud_storage_password);
	}
	GET_INT("cloud_verification_status", cloud_verification_status);
	GET_BOOL("git_local_only", git_local_only);

	// creating the git url here is simply a convenience when C code wants
	// to compare against that git URL - it's always derived from the base URL
	GET_TXT("cloud_base_url", cloud_base_url);
	prefs.cloud_git_url = copy_qstring(QString(prefs.cloud_base_url) + "/git");
	s.endGroup();

	// Subsurface webservice id is stored outside of the groups
	GET_TXT("subsurface_webservice_uid", userid);

	// GeoManagement
	s.beginGroup("geocoding");

	GET_ENUM("cat0", taxonomy_category, geocoding.category[0]);
	GET_ENUM("cat1", taxonomy_category, geocoding.category[1]);
	GET_ENUM("cat2", taxonomy_category, geocoding.category[2]);
	s.endGroup();

	// GPS service time and distance thresholds
	s.beginGroup("LocationService");
	GET_INT("time_threshold", time_threshold);
	GET_INT("distance_threshold", distance_threshold);
	s.endGroup();

	s.beginGroup("Planner");
	GET_BOOL("last_stop", last_stop);
	GET_BOOL("verbatim_plan", verbatim_plan);
	GET_BOOL("display_duration", display_duration);
	GET_BOOL("display_runtime", display_runtime);
	GET_BOOL("display_transitions", display_transitions);
	GET_BOOL("display_variations", display_variations);
	GET_BOOL("safetystop", safetystop);
	GET_BOOL("doo2breaks", doo2breaks);
	GET_BOOL("switch_at_req_stop",switch_at_req_stop);
	GET_BOOL("drop_stone_mode", drop_stone_mode);

	GET_INT("reserve_gas", reserve_gas);
	GET_INT("ascrate75", ascrate75);
	GET_INT("ascrate50", ascrate50);
	GET_INT("ascratestops", ascratestops);
	GET_INT("ascratelast6m", ascratelast6m);
	GET_INT("descrate", descrate);
	GET_INT("sacfactor", sacfactor);
	GET_INT("problemsolvingtime", problemsolvingtime);
	GET_INT("bottompo2", bottompo2);
	GET_INT("decopo2", decopo2);
	GET_INT("bestmixend", bestmixend.mm);
	GET_INT("min_switch_duration", min_switch_duration);
	GET_INT("bottomsac", bottomsac);
	GET_INT("decosac", decosac);

	prefs.planner_deco_mode = deco_mode(s.value("deco_mode", default_prefs.planner_deco_mode).toInt());
	s.endGroup();

	s.beginGroup("DiveComputer");
	GET_TXT("dive_computer_vendor",dive_computer.vendor);
	GET_TXT("dive_computer_product", dive_computer.product);
	GET_TXT("dive_computer_device", dive_computer.device);
	GET_TXT("dive_computer_device_name", dive_computer.device_name);
	GET_INT("dive_computer_download_mode", dive_computer.download_mode);
	s.endGroup();

	s.beginGroup("UpdateManager");
	prefs.update_manager.dont_check_exists = s.contains("DontCheckForUpdates");
	GET_BOOL("DontCheckForUpdates", update_manager.dont_check_for_updates);
	GET_TXT("LastVersionUsed", update_manager.last_version_used);
	prefs.update_manager.next_check = copy_qstring(s.value("NextCheck").toDate().toString("dd/MM/yyyy"));
	s.endGroup();

	s.beginGroup("Language");
	GET_BOOL("UseSystemLanguage", locale.use_system_language);
	GET_TXT("UiLanguage", locale.language);
	GET_TXT("UiLangLocale", locale.lang_locale);
	GET_TXT("time_format", time_format);
	GET_TXT("date_format", date_format);
	GET_TXT("date_format_short", date_format_short);
	GET_BOOL("time_format_override", time_format_override);
	GET_BOOL("date_format_override", date_format_override);
	s.endGroup();
}

void SettingsObjectWrapper::sync()
{
	QSettings s;
	s.beginGroup("Planner");
	s.setValue("last_stop", prefs.last_stop);
	s.setValue("verbatim_plan", prefs.verbatim_plan);
	s.setValue("display_duration", prefs.display_duration);
	s.setValue("display_runtime", prefs.display_runtime);
	s.setValue("display_transitions", prefs.display_transitions);
	s.setValue("display_variations", prefs.display_variations);
	s.setValue("safetystop", prefs.safetystop);
	s.setValue("reserve_gas", prefs.reserve_gas);
	s.setValue("ascrate75", prefs.ascrate75);
	s.setValue("ascrate50", prefs.ascrate50);
	s.setValue("ascratestops", prefs.ascratestops);
	s.setValue("ascratelast6m", prefs.ascratelast6m);
	s.setValue("descrate", prefs.descrate);
	s.setValue("sacfactor", prefs.sacfactor);
	s.setValue("problemsolvingtime", prefs.problemsolvingtime);
	s.setValue("bottompo2", prefs.bottompo2);
	s.setValue("decopo2", prefs.decopo2);
	s.setValue("bestmixend", prefs.bestmixend.mm);
	s.setValue("doo2breaks", prefs.doo2breaks);
	s.setValue("drop_stone_mode", prefs.drop_stone_mode);
	s.setValue("switch_at_req_stop", prefs.switch_at_req_stop);
	s.setValue("min_switch_duration", prefs.min_switch_duration);
	s.setValue("bottomsac", prefs.bottomsac);
	s.setValue("decosac", prefs.decosac);
	s.setValue("deco_mode", int(prefs.planner_deco_mode));
	s.endGroup();
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
