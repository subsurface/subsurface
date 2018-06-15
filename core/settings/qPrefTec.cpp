// SPDX-License-Identifier: GPL-2.0
#include "qPrefTec.h"
#include "qPref_private.h"




qPrefTec *qPrefTec::m_instance = NULL;
qPrefTec *qPrefTec::instance()
{
	if (!m_instance)
		m_instance = new qPrefTec;
	return m_instance;
}


void qPrefTec::loadSync(bool doSync)
{
	diskCalcalltissues(doSync);
	diskCalcceiling(doSync);
	diskCalcceiling3m(doSync);
	diskCalcndltts(doSync);
	diskDCceiling(doSync);
	diskDecoMode(doSync);
	diskDisplayUnusedTanks(doSync);
	diskEad(doSync);
	diskGfhigh(doSync);
	diskGflow(doSync);
	diskGflowAtMaxDepth(doSync);
	diskHRgraph(doSync);
	diskMod(doSync);
	diskModpO2(doSync);
	diskPercentageGraph(doSync);
	diskRedceiling(doSync);
	diskRulerGraph(doSync);
	diskShowAverageDepth(doSync);
	diskShowCCRSensors(doSync);
	diskShowCCRSetpoint(doSync);
	diskShowIcd(doSync);
	diskShowPicturesInProfile(doSync);
	diskShowSac(doSync);
	diskShowSCROCpO2(doSync);
	diskTankBar(doSync);
	diskVpmbConservatism(doSync);
	diskZoomedPlot(doSync);
}

bool qPrefTec::buehlmann() const
{
	return prefs.planner_deco_mode == BUEHLMANN;
}
void qPrefTec::setBuehlmann(bool value)
{
	if (value != (prefs.planner_deco_mode == BUEHLMANN)) {
		prefs.planner_deco_mode = value ? BUEHLMANN : VPMB;
		emit buehlmannChanged(value);
	}

	// value is NOT stored on disk, because it is indirect
}


bool qPrefTec::calcalltissues() const
{
	return prefs.calcalltissues;
}
void qPrefTec::setCalcalltissues(bool value)
{
	if (value != prefs.calcalltissues) {
		prefs.calcalltissues = value;
		diskCalcalltissues(true);
		emit calcalltissuesChanged(value);
	}
}
void qPrefTec::diskCalcalltissues(bool doSync)
{
	LOADSYNC_BOOL("/calcalltissues", calcalltissues);
}


bool qPrefTec::calcceiling() const
{
	return prefs.calcceiling;
}
void qPrefTec::setCalcceiling(bool value)
{
	if (value != prefs.calcceiling) {
		prefs.calcceiling = value;
		diskCalcceiling(true);
		emit calcceilingChanged(value);
	}
}
void qPrefTec::diskCalcceiling(bool doSync)
{
	LOADSYNC_BOOL("/calcceiling", calcceiling);
}


bool qPrefTec::calcceiling3m() const
{
	return prefs.calcceiling3m;
}
void qPrefTec::setCalcceiling3m(bool value)
{
	if (value != prefs.calcceiling3m) {
		prefs.calcceiling3m = value;
		diskCalcceiling3m(true);
		emit calcceiling3mChanged(true);
	}
}
void qPrefTec::diskCalcceiling3m(bool doSync)
{
	LOADSYNC_BOOL("/calcceiling3m", calcceiling3m);
}


bool qPrefTec::calcndltts() const
{
	return prefs.calcndltts;
}
void qPrefTec::setCalcndltts(bool value)
{
	if (value != prefs.calcndltts) {
		prefs.calcndltts = value;
		diskCalcndltts(true);
		emit calcndlttsChanged(value);
	}
}
void qPrefTec::diskCalcndltts(bool doSync)
{
	LOADSYNC_BOOL("/calcndltts", calcndltts);
}


bool qPrefTec::dcceiling() const
{
	return prefs.dcceiling;
}
void qPrefTec::setDCceiling(bool value)
{
	if (value != prefs.dcceiling) {
		prefs.dcceiling = value;
		diskDCceiling(true);
		emit dcceilingChanged(value);
	}
}
void qPrefTec::diskDCceiling(bool doSync)
{
	LOADSYNC_BOOL("/dcceiling", dcceiling);
}


deco_mode qPrefTec::decoMode() const
{
	return prefs.display_deco_mode;
}
void qPrefTec::setDecoMode(deco_mode value)
{
	if (prefs.display_deco_mode != value) {
		prefs.display_deco_mode = value;
		diskDecoMode(true);
		emit decoModeChanged(value);
	}
}
void qPrefTec::diskDecoMode(bool doSync)
{
	LOADSYNC_ENUM("/display_deco_mode", deco_mode, display_deco_mode);
}



bool qPrefTec::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}
void qPrefTec::setDisplayUnusedTanks(bool value)
{
	if (value != prefs.display_unused_tanks) {
		prefs.display_unused_tanks = value;
		diskDisplayUnusedTanks(true);
		emit displayUnusedTanksChanged(value);
	}
}
void qPrefTec::diskDisplayUnusedTanks(bool doSync)
{
	LOADSYNC_BOOL("/display_unused_tanks", display_unused_tanks);
}


bool qPrefTec::ead() const
{
	return prefs.ead;
}
void qPrefTec::setEad(bool value)
{
	if (value != prefs.ead) {
		prefs.ead = value;
		diskEad(true);
		emit eadChanged(value);
	}
}
void qPrefTec::diskEad(bool doSync)
{
	LOADSYNC_BOOL("/ead", ead);
}



int qPrefTec::gfhigh() const
{
	return prefs.gfhigh;
}
void qPrefTec::setGfhigh(int value)
{
	if (value != prefs.gfhigh) {
		prefs.gfhigh = value;
		set_gf(prefs.gflow, prefs.gfhigh);
		diskGfhigh(true);
		emit gfhighChanged(value);
	}
}
void qPrefTec::diskGfhigh(bool doSync)
{
	LOADSYNC_INT("/gfhigh", gfhigh);
}


int qPrefTec::gflow() const
{
	return prefs.gflow;
}
void qPrefTec::setGflow(int value)
{
	if (value != prefs.gflow) {
		prefs.gflow = value;
		set_gf(prefs.gflow, prefs.gfhigh);
		diskGflow(true);
		emit gflowChanged(value);
	}
}
void qPrefTec::diskGflow(bool doSync)
{
	LOADSYNC_INT("/gflow", gflow);
}


int qPrefTec::gflowAtMaxDepth() const
{
	return prefs.gflow;
}
void qPrefTec::setGflowAtMaxDepth(int value)
{
	if (value != prefs.gf_low_at_maxdepth) {
		prefs.gf_low_at_maxdepth = value;
		diskGflowAtMaxDepth(true);
		emit gflowAtMaxDepthChanged(value);
	}
}
void qPrefTec::diskGflowAtMaxDepth(bool doSync)
{
	LOADSYNC_INT("/gf_low_at_maxdepth", gf_low_at_maxdepth);
}



bool qPrefTec::hrgraph() const
{
	return prefs.hrgraph;
}
void qPrefTec::setHRgraph(bool value)
{
	if (value != prefs.hrgraph) {
		prefs.hrgraph = value;
		diskHRgraph(true);
		emit hrgraphChanged(value);
	}
}
void qPrefTec::diskHRgraph(bool doSync)
{
	LOADSYNC_BOOL("/hrgraph", hrgraph);
}


bool qPrefTec::mod() const
{
	return prefs.mod;
}
void qPrefTec::setMod(bool value)
{
	if (value != prefs.mod) {
		prefs.mod = value;
		diskMod(true);
		emit modChanged(value);
	}
}
void qPrefTec::diskMod(bool doSync)
{
	LOADSYNC_BOOL("/mod", mod);
}


double qPrefTec:: modpO2() const
{
	return prefs.modpO2;
}
void qPrefTec::setModpO2(double value)
{
	if (value != prefs.modpO2) {
		prefs.modpO2 = value;
		diskModpO2(true);
		emit modpO2Changed(value);
	}
}
void qPrefTec::diskModpO2(bool doSync)
{
	LOADSYNC_DOUBLE("/modpO2", modpO2);
}


bool qPrefTec::percentageGraph() const
{
	return prefs.percentagegraph;
}
void qPrefTec::setPercentageGraph(bool value)
{
	if (value != prefs.percentagegraph) {
		prefs.percentagegraph = value;
		diskPercentageGraph(true);
		emit percentageGraphChanged(value);
	}
}
void qPrefTec::diskPercentageGraph(bool doSync)
{
	LOADSYNC_BOOL("/percentagegraph", percentagegraph);
}


bool qPrefTec::redceiling() const
{
	return prefs.redceiling;
}
void qPrefTec::setRedceiling(bool value)
{
	if (value != prefs.redceiling) {
		prefs.redceiling = value;
		diskRedceiling(true);
		emit redceilingChanged(value);
	}
}
void qPrefTec::diskRedceiling(bool doSync)
{
	LOADSYNC_BOOL("/redceiling", redceiling);
}


bool qPrefTec::rulerGraph() const
{
	return prefs.rulergraph;
}
void qPrefTec::setRulerGraph(bool value)
{
	if (value != prefs.rulergraph) {
		prefs.rulergraph = value;
		diskRulerGraph(true);
		emit rulerGraphChanged(value);
	}
}
void qPrefTec::diskRulerGraph(bool doSync)
{
	LOADSYNC_BOOL("/RulerBar", rulergraph);
}


bool qPrefTec::showAverageDepth() const
{
	return prefs.show_average_depth;
}
void qPrefTec::setShowAverageDepth(bool value)
{
	if (value != prefs.show_average_depth) {
		prefs.show_average_depth = value;
		diskShowAverageDepth(true);
		emit showAverageDepthChanged(true);
	}
}
void qPrefTec::diskShowAverageDepth(bool doSync)
{
	LOADSYNC_BOOL("/show_average_depth", show_average_depth);
}


bool qPrefTec::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}
void qPrefTec::setShowCCRSensors(bool value)
{
	if (value != prefs.show_ccr_sensors) {
		prefs.show_ccr_sensors = value;
		diskShowCCRSensors(true);
		emit showCCRSensorsChanged(value);
	}
}
void qPrefTec::diskShowCCRSensors(bool doSync)
{
	LOADSYNC_BOOL("/show_ccr_sensors", show_ccr_sensors);
}


bool qPrefTec::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}
void qPrefTec::setShowCCRSetpoint(bool value)
{
	if (value != prefs.show_ccr_setpoint) {
		prefs.show_ccr_setpoint = value;
		diskShowCCRSetpoint(true);
		emit showCCRSetpointChanged(value);
	}
}
void qPrefTec::diskShowCCRSetpoint(bool doSync)
{
	LOADSYNC_BOOL("/show_ccr_setpoint", show_ccr_setpoint);
}


bool qPrefTec::showIcd() const
{
	return prefs.show_icd;
}
void qPrefTec::setShowIcd(bool value)
{
	if (value != prefs.show_icd) {
		prefs.show_icd = value;
		diskShowIcd(true);
		emit showIcdChanged(value);
	}
}
void qPrefTec::diskShowIcd(bool doSync)
{
	LOADSYNC_BOOL("/show_icd", show_icd);
}


bool qPrefTec::showPicturesInProfile() const
{
	return prefs.show_pictures_in_profile;
}
void qPrefTec::setShowPicturesInProfile(bool value)
{
	if (value != prefs.show_pictures_in_profile) {
		prefs.show_pictures_in_profile = value;
		diskShowPicturesInProfile(true);
		emit showPicturesInProfileChanged(value);
	}
}
void qPrefTec::diskShowPicturesInProfile(bool doSync)
{
	LOADSYNC_BOOL("/show_pictures_in_profile", show_pictures_in_profile);
}


bool qPrefTec::showSac() const
{
	return prefs.show_sac;
}
void qPrefTec::setShowSac(bool value)
{
	if (value != prefs.show_sac) {
		prefs.show_sac = value;
		diskShowSac(true);
		emit showSacChanged(value);
	}
}
void qPrefTec::diskShowSac(bool doSync)
{
	LOADSYNC_BOOL("/show_sac", show_sac);
}


bool qPrefTec::showSCROCpO2() const
{
	return prefs.show_scr_ocpo2;
}
void qPrefTec::setShowSCROCpO2(bool value)
{
	if (value != prefs.show_scr_ocpo2) {
		prefs.show_scr_ocpo2 = value;
		diskShowSCROCpO2(true);
		emit showSCROCpO2Changed(value);
	}
}
void qPrefTec::diskShowSCROCpO2(bool doSync)
{
	LOADSYNC_BOOL("/show_scr_ocpo2", show_scr_ocpo2);
}


bool qPrefTec::tankBar() const
{
	return prefs.tankbar;
}
void qPrefTec::setTankBar(bool value)
{
	if (value != prefs.tankbar) {
		prefs.tankbar = value;
		diskTankBar(true);
		emit tankBarChanged(value);
	}
}
void qPrefTec::diskTankBar(bool doSync)
{
	LOADSYNC_BOOL("/tankbar", tankbar);
}


short qPrefTec::vpmbConservatism() const
{
	return prefs.vpmb_conservatism;
}
void qPrefTec::setVpmbConservatism(short value)
{
	if (value != prefs.vpmb_conservatism) {
		prefs.vpmb_conservatism = value;
		set_vpmb_conservatism(value);
		diskVpmbConservatism(true);
		emit vpmbConservatismChanged(value);
	}
}
void qPrefTec::diskVpmbConservatism(bool doSync)
{
	LOADSYNC_INT("/vpmb_conservatism", vpmb_conservatism);
}


bool qPrefTec::zoomedPlot() const
{
	return prefs.zoomed_plot;
}
void qPrefTec::setZoomedPlot(bool value)
{
	if (value != prefs.zoomed_plot) {
		prefs.zoomed_plot = value;
		diskZoomedPlot(true);
		emit zoomedPlotChanged(value);
	}
}
void qPrefTec::diskZoomedPlot(bool doSync)
{
	LOADSYNC_BOOL("/zoomed_plot", zoomed_plot);
}
