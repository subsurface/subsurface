// SPDX-License-Identifier: GPL-2.0
#include "qPrefTechnicalDetails.h"
#include "qPref_private.h"




qPrefTechnicalDetails *qPrefTechnicalDetails::m_instance = NULL;
qPrefTechnicalDetails *qPrefTechnicalDetails::instance()
{
	if (!m_instance)
		m_instance = new qPrefTechnicalDetails;
	return m_instance;
}


void qPrefTechnicalDetails::loadSync(bool doSync)
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

bool qPrefTechnicalDetails::buehlmann() const
{
	return prefs.planner_deco_mode == BUEHLMANN;
}
void qPrefTechnicalDetails::setBuehlmann(bool value)
{
	if (value != (prefs.planner_deco_mode == BUEHLMANN)) {
		prefs.planner_deco_mode = value ? BUEHLMANN : VPMB;
		emit buehlmannChanged(value);
	}

	// value is NOT stored on disk, because it is indirect
}


bool qPrefTechnicalDetails::calcalltissues() const
{
	return prefs.calcalltissues;
}
void qPrefTechnicalDetails::setCalcalltissues(bool value)
{
	if (value != prefs.calcalltissues) {
		prefs.calcalltissues = value;
		diskCalcalltissues(true);
		emit calcalltissuesChanged(value);
	}
}
void qPrefTechnicalDetails::diskCalcalltissues(bool doSync)
{
	LOADSYNC_BOOL("/calcalltissues", calcalltissues);
}


bool qPrefTechnicalDetails::calcceiling() const
{
	return prefs.calcceiling;
}
void qPrefTechnicalDetails::setCalcceiling(bool value)
{
	if (value != prefs.calcceiling) {
		prefs.calcceiling = value;
		diskCalcceiling(true);
		emit calcceilingChanged(value);
	}
}
void qPrefTechnicalDetails::diskCalcceiling(bool doSync)
{
	LOADSYNC_BOOL("/calcceiling", calcceiling);
}


bool qPrefTechnicalDetails::calcceiling3m() const
{
	return prefs.calcceiling3m;
}
void qPrefTechnicalDetails::setCalcceiling3m(bool value)
{
	if (value != prefs.calcceiling3m) {
		prefs.calcceiling3m = value;
		diskCalcceiling3m(true);
		emit calcceiling3mChanged(true);
	}
}
void qPrefTechnicalDetails::diskCalcceiling3m(bool doSync)
{
	LOADSYNC_BOOL("/calcceiling3m", calcceiling3m);
}


bool qPrefTechnicalDetails::calcndltts() const
{
	return prefs.calcndltts;
}
void qPrefTechnicalDetails::setCalcndltts(bool value)
{
	if (value != prefs.calcndltts) {
		prefs.calcndltts = value;
		diskCalcndltts(true);
		emit calcndlttsChanged(value);
	}
}
void qPrefTechnicalDetails::diskCalcndltts(bool doSync)
{
	LOADSYNC_BOOL("/calcndltts", calcndltts);
}


bool qPrefTechnicalDetails::dcceiling() const
{
	return prefs.dcceiling;
}
void qPrefTechnicalDetails::setDCceiling(bool value)
{
	if (value != prefs.dcceiling) {
		prefs.dcceiling = value;
		diskDCceiling(true);
		emit dcceilingChanged(value);
	}
}
void qPrefTechnicalDetails::diskDCceiling(bool doSync)
{
	LOADSYNC_BOOL("/dcceiling", dcceiling);
}


deco_mode qPrefTechnicalDetails::decoMode() const
{
	return prefs.display_deco_mode;
}
void qPrefTechnicalDetails::setDecoMode(deco_mode value)
{
	if (prefs.display_deco_mode != value) {
		prefs.display_deco_mode = value;
		diskDecoMode(true);
		emit decoModeChanged(value);
	}
}
void qPrefTechnicalDetails::diskDecoMode(bool doSync)
{
	LOADSYNC_ENUM("/display_deco_mode", deco_mode, display_deco_mode);
}



bool qPrefTechnicalDetails::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}
void qPrefTechnicalDetails::setDisplayUnusedTanks(bool value)
{
	if (value != prefs.display_unused_tanks) {
		prefs.display_unused_tanks = value;
		diskDisplayUnusedTanks(true);
		emit displayUnusedTanksChanged(value);
	}
}
void qPrefTechnicalDetails::diskDisplayUnusedTanks(bool doSync)
{
	LOADSYNC_BOOL("/display_unused_tanks", display_unused_tanks);
}


bool qPrefTechnicalDetails::ead() const
{
	return prefs.ead;
}
void qPrefTechnicalDetails::setEad(bool value)
{
	if (value != prefs.ead) {
		prefs.ead = value;
		diskEad(true);
		emit eadChanged(value);
	}
}
void qPrefTechnicalDetails::diskEad(bool doSync)
{
	LOADSYNC_BOOL("/ead", ead);
}



int qPrefTechnicalDetails::gfhigh() const
{
	return prefs.gfhigh;
}
void qPrefTechnicalDetails::setGfhigh(int value)
{
	if (value != prefs.gfhigh) {
		prefs.gfhigh = value;
		set_gf(prefs.gflow, prefs.gfhigh);
		diskGfhigh(true);
		emit gfhighChanged(value);
	}
}
void qPrefTechnicalDetails::diskGfhigh(bool doSync)
{
	LOADSYNC_INT("/gfhigh", gfhigh);
}


int qPrefTechnicalDetails::gflow() const
{
	return prefs.gflow;
}
void qPrefTechnicalDetails::setGflow(int value)
{
	if (value != prefs.gflow) {
		prefs.gflow = value;
		set_gf(prefs.gflow, prefs.gfhigh);
		diskGflow(true);
		emit gflowChanged(value);
	}
}
void qPrefTechnicalDetails::diskGflow(bool doSync)
{
	LOADSYNC_INT("/gflow", gflow);
}


int qPrefTechnicalDetails::gflowAtMaxDepth() const
{
	return prefs.gflow;
}
void qPrefTechnicalDetails::setGflowAtMaxDepth(int value)
{
	if (value != prefs.gf_low_at_maxdepth) {
		prefs.gf_low_at_maxdepth = value;
		diskGflowAtMaxDepth(true);
		emit gflowAtMaxDepthChanged(value);
	}
}
void qPrefTechnicalDetails::diskGflowAtMaxDepth(bool doSync)
{
	LOADSYNC_INT("/gf_low_at_maxdepth", gf_low_at_maxdepth);
}



bool qPrefTechnicalDetails::hrgraph() const
{
	return prefs.hrgraph;
}
void qPrefTechnicalDetails::setHRgraph(bool value)
{
	if (value != prefs.hrgraph) {
		prefs.hrgraph = value;
		diskHRgraph(true);
		emit hrgraphChanged(value);
	}
}
void qPrefTechnicalDetails::diskHRgraph(bool doSync)
{
	LOADSYNC_BOOL("/hrgraph", hrgraph);
}


bool qPrefTechnicalDetails::mod() const
{
	return prefs.mod;
}
void qPrefTechnicalDetails::setMod(bool value)
{
	if (value != prefs.mod) {
		prefs.mod = value;
		diskMod(true);
		emit modChanged(value);
	}
}
void qPrefTechnicalDetails::diskMod(bool doSync)
{
	LOADSYNC_BOOL("/mod", mod);
}


double qPrefTechnicalDetails:: modpO2() const
{
	return prefs.modpO2;
}
void qPrefTechnicalDetails::setModpO2(double value)
{
	if (value != prefs.modpO2) {
		prefs.modpO2 = value;
		diskModpO2(true);
		emit modpO2Changed(value);
	}
}
void qPrefTechnicalDetails::diskModpO2(bool doSync)
{
	LOADSYNC_DOUBLE("/modpO2", modpO2);
}


bool qPrefTechnicalDetails::percentageGraph() const
{
	return prefs.percentagegraph;
}
void qPrefTechnicalDetails::setPercentageGraph(bool value)
{
	if (value != prefs.percentagegraph) {
		prefs.percentagegraph = value;
		diskPercentageGraph(true);
		emit percentageGraphChanged(value);
	}
}
void qPrefTechnicalDetails::diskPercentageGraph(bool doSync)
{
	LOADSYNC_BOOL("/percentagegraph", percentagegraph);
}


bool qPrefTechnicalDetails::redceiling() const
{
	return prefs.redceiling;
}
void qPrefTechnicalDetails::setRedceiling(bool value)
{
	if (value != prefs.redceiling) {
		prefs.redceiling = value;
		diskRedceiling(true);
		emit redceilingChanged(value);
	}
}
void qPrefTechnicalDetails::diskRedceiling(bool doSync)
{
	LOADSYNC_BOOL("/redceiling", redceiling);
}


bool qPrefTechnicalDetails::rulerGraph() const
{
	return prefs.rulergraph;
}
void qPrefTechnicalDetails::setRulerGraph(bool value)
{
	if (value != prefs.rulergraph) {
		prefs.rulergraph = value;
		diskRulerGraph(true);
		emit rulerGraphChanged(value);
	}
}
void qPrefTechnicalDetails::diskRulerGraph(bool doSync)
{
	LOADSYNC_BOOL("/RulerBar", rulergraph);
}


bool qPrefTechnicalDetails::showAverageDepth() const
{
	return prefs.show_average_depth;
}
void qPrefTechnicalDetails::setShowAverageDepth(bool value)
{
	if (value != prefs.show_average_depth) {
		prefs.show_average_depth = value;
		diskShowAverageDepth(true);
		emit showAverageDepthChanged(true);
	}
}
void qPrefTechnicalDetails::diskShowAverageDepth(bool doSync)
{
	LOADSYNC_BOOL("/show_average_depth", show_average_depth);
}


bool qPrefTechnicalDetails::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}
void qPrefTechnicalDetails::setShowCCRSensors(bool value)
{
	if (value != prefs.show_ccr_sensors) {
		prefs.show_ccr_sensors = value;
		diskShowCCRSensors(true);
		emit showCCRSensorsChanged(value);
	}
}
void qPrefTechnicalDetails::diskShowCCRSensors(bool doSync)
{
	LOADSYNC_BOOL("/show_ccr_sensors", show_ccr_sensors);
}


bool qPrefTechnicalDetails::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}
void qPrefTechnicalDetails::setShowCCRSetpoint(bool value)
{
	if (value != prefs.show_ccr_setpoint) {
		prefs.show_ccr_setpoint = value;
		diskShowCCRSetpoint(true);
		emit showCCRSetpointChanged(value);
	}
}
void qPrefTechnicalDetails::diskShowCCRSetpoint(bool doSync)
{
	LOADSYNC_BOOL("/show_ccr_setpoint", show_ccr_setpoint);
}


bool qPrefTechnicalDetails::showIcd() const
{
	return prefs.show_icd;
}
void qPrefTechnicalDetails::setShowIcd(bool value)
{
	if (value != prefs.show_icd) {
		prefs.show_icd = value;
		diskShowIcd(true);
		emit showIcdChanged(value);
	}
}
void qPrefTechnicalDetails::diskShowIcd(bool doSync)
{
	LOADSYNC_BOOL("/show_icd", show_icd);
}


bool qPrefTechnicalDetails::showPicturesInProfile() const
{
	return prefs.show_pictures_in_profile;
}
void qPrefTechnicalDetails::setShowPicturesInProfile(bool value)
{
	if (value != prefs.show_pictures_in_profile) {
		prefs.show_pictures_in_profile = value;
		diskShowPicturesInProfile(true);
		emit showPicturesInProfileChanged(value);
	}
}
void qPrefTechnicalDetails::diskShowPicturesInProfile(bool doSync)
{
	LOADSYNC_BOOL("/show_pictures_in_profile", show_pictures_in_profile);
}


bool qPrefTechnicalDetails::showSac() const
{
	return prefs.show_sac;
}
void qPrefTechnicalDetails::setShowSac(bool value)
{
	if (value != prefs.show_sac) {
		prefs.show_sac = value;
		diskShowSac(true);
		emit showSacChanged(value);
	}
}
void qPrefTechnicalDetails::diskShowSac(bool doSync)
{
	LOADSYNC_BOOL("/show_sac", show_sac);
}


bool qPrefTechnicalDetails::showSCROCpO2() const
{
	return prefs.show_scr_ocpo2;
}
void qPrefTechnicalDetails::setShowSCROCpO2(bool value)
{
	if (value != prefs.show_scr_ocpo2) {
		prefs.show_scr_ocpo2 = value;
		diskShowSCROCpO2(true);
		emit showSCROCpO2Changed(value);
	}
}
void qPrefTechnicalDetails::diskShowSCROCpO2(bool doSync)
{
	LOADSYNC_BOOL("/show_scr_ocpo2", show_scr_ocpo2);
}


bool qPrefTechnicalDetails::tankBar() const
{
	return prefs.tankbar;
}
void qPrefTechnicalDetails::setTankBar(bool value)
{
	if (value != prefs.tankbar) {
		prefs.tankbar = value;
		diskTankBar(true);
		emit tankBarChanged(value);
	}
}
void qPrefTechnicalDetails::diskTankBar(bool doSync)
{
	LOADSYNC_BOOL("/tankbar", tankbar);
}


short qPrefTechnicalDetails::vpmbConservatism() const
{
	return prefs.vpmb_conservatism;
}
void qPrefTechnicalDetails::setVpmbConservatism(short value)
{
	if (value != prefs.vpmb_conservatism) {
		prefs.vpmb_conservatism = value;
		set_vpmb_conservatism(value);
		diskVpmbConservatism(true);
		emit vpmbConservatismChanged(value);
	}
}
void qPrefTechnicalDetails::diskVpmbConservatism(bool doSync)
{
	LOADSYNC_INT("/vpmb_conservatism", vpmb_conservatism);
}


bool qPrefTechnicalDetails::zoomedPlot() const
{
	return prefs.zoomed_plot;
}
void qPrefTechnicalDetails::setZoomedPlot(bool value)
{
	if (value != prefs.zoomed_plot) {
		prefs.zoomed_plot = value;
		diskZoomedPlot(true);
		emit zoomedPlotChanged(value);
	}
}
void qPrefTechnicalDetails::diskZoomedPlot(bool doSync)
{
	LOADSYNC_BOOL("/zoomed_plot", zoomed_plot);
}
