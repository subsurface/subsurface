#include "SettingsObjectWrapper.h"
#include <QSettings>

static QString tecDetails = QStringLiteral("TecDetails");

short PartialPressureGasSettings::showPo2() const
{
	return prefs.pp_graphs.po2;
}

short PartialPressureGasSettings::showPn2() const
{
	return prefs.pp_graphs.pn2;
}

short PartialPressureGasSettings::showPhe() const
{
	return prefs.pp_graphs.phe;
}

double PartialPressureGasSettings::po2Threshold() const
{
	return prefs.pp_graphs.po2_threshold;
}

double PartialPressureGasSettings::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}

double PartialPressureGasSettings::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}

double PartialPressureGasSettings:: modp02() const
{
	return prefs.modpO2;
}

short PartialPressureGasSettings::ead() const
{
	return prefs.ead;
}

short PartialPressureGasSettings::dcceiling() const
{
	return prefs.dcceiling;
}

short PartialPressureGasSettings::redceiling() const
{
	return prefs.redceiling;
}

short PartialPressureGasSettings::calcceiling() const
{
	return prefs.calcceiling;
}

short PartialPressureGasSettings::calcceiling3m() const
{
	return prefs.calcceiling3m;
}

short PartialPressureGasSettings::calcalltissues() const
{
	return prefs.calcalltissues;
}

short PartialPressureGasSettings::calcndltts() const
{
	return prefs.calcndltts;
}

short PartialPressureGasSettings::gflow() const
{
	return prefs.gflow;
}

short PartialPressureGasSettings::gfhigh() const
{
	return prefs.gfhigh;
}

short PartialPressureGasSettings::hrgraph() const
{
	return prefs.hrgraph;
}

short PartialPressureGasSettings::tankBar() const
{
	return prefs.tankbar;
}

short PartialPressureGasSettings::percentageGraph() const
{
	return prefs.percentagegraph;
}

short PartialPressureGasSettings::rulerGraph() const
{
	return prefs.rulergraph;
}

bool PartialPressureGasSettings::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}

bool PartialPressureGasSettings::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}

short PartialPressureGasSettings::zoomedPlot() const
{
	return prefs.zoomed_plot;
}

short PartialPressureGasSettings::showSac() const
{
	return prefs.show_sac;
}

bool PartialPressureGasSettings::gfLowAtMaxDepth() const
{
	return prefs.gf_low_at_maxdepth;
}

short PartialPressureGasSettings::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}

short PartialPressureGasSettings::showAverageDepth() const
{
	return prefs.show_average_depth;
}

void PartialPressureGasSettings::setShowPo2(short value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("po2graph", value);
	prefs.pp_graphs.po2 = value;
	emit showPo2Changed(value);
}

void PartialPressureGasSettings::setShowPn2(short value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("pn2graph", value);
	prefs.pp_graphs.pn2 = value;
	emit showPn2Changed(value);
}

void PartialPressureGasSettings::setShowPhe(short value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("phegraph", value);
	prefs.pp_graphs.phe = value;
	emit showPheChanged(value);
}

void PartialPressureGasSettings::setPo2Threshold(double value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("po2threshold", value);
	prefs.pp_graphs.po2_threshold = value;
	emit po2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPn2Threshold(double value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("pn2threshold", value);
	prefs.pp_graphs.pn2_threshold = value;
	emit pn2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPheThreshold(double value)
{
	QSettings s;
	s.beginGroup(tecDetails);
	s.setValue("phethreshold", value);
	prefs.pp_graphs.phe_threshold = value;
	emit pheThresholdChanged(value);
}
