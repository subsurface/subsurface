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
