// SPDX-License-Identifier: GPL-2.0
#include "qPrefPartialPressureGas.h"
#include "qPref_private.h"



qPrefPartialPressureGas *qPrefPartialPressureGas::m_instance = NULL;
qPrefPartialPressureGas *qPrefPartialPressureGas::instance()
{
	if (!m_instance)
		m_instance = new qPrefPartialPressureGas;
	return m_instance;
}

void qPrefPartialPressureGas::loadSync(bool doSync)
{
	diskPheThreshold(doSync);
	diskPn2Threshold(doSync);
	diskPo2ThresholdMin(doSync);
	diskPo2ThresholdMax(doSync);
	diskShowPhe(doSync);
	diskShowPn2(doSync);
	diskShowPo2(doSync);
}


double qPrefPartialPressureGas::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}
void qPrefPartialPressureGas::setPheThreshold(double value)
{
	if (value != prefs.pp_graphs.phe_threshold) {
		prefs.pp_graphs.phe_threshold = value;
		diskPheThreshold(true);
		emit pheThresholdChanged(value);
	}
}
void qPrefPartialPressureGas::diskPheThreshold(bool doSync)
{
	LOADSYNC_DOUBLE("/phethreshold", pp_graphs.phe_threshold);
}


double qPrefPartialPressureGas::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}
void qPrefPartialPressureGas::setPn2Threshold(double value)
{
	if (value != prefs.pp_graphs.pn2_threshold) {
		prefs.pp_graphs.pn2_threshold = value;
		diskPn2Threshold(true);
		emit pn2ThresholdChanged(value);
	}
}
void qPrefPartialPressureGas::diskPn2Threshold(bool doSync)
{
	LOADSYNC_DOUBLE("/pn2threshold", pp_graphs.pn2_threshold);
}


double qPrefPartialPressureGas::po2ThresholdMax() const
{
	return prefs.pp_graphs.po2_threshold_max;
}
void qPrefPartialPressureGas::setPo2ThresholdMax(double value)
{
	if (value != prefs.pp_graphs.po2_threshold_max) {
		prefs.pp_graphs.po2_threshold_max = value;
		diskPo2ThresholdMax(true);
		emit po2ThresholdMaxChanged(value);
	}
}
void qPrefPartialPressureGas::diskPo2ThresholdMax(bool doSync)
{
	LOADSYNC_DOUBLE("/po2thresholdmax", pp_graphs.po2_threshold_max);
}


double qPrefPartialPressureGas::po2ThresholdMin() const
{
	return prefs.pp_graphs.po2_threshold_min;
}
void qPrefPartialPressureGas::setPo2ThresholdMin(double value)
{
	if (value != prefs.pp_graphs.po2_threshold_min) {
		prefs.pp_graphs.po2_threshold_min = value;
		emit po2ThresholdMinChanged(value);
	}
}
void qPrefPartialPressureGas::diskPo2ThresholdMin(bool doSync)
{
	LOADSYNC_DOUBLE("/po2thresholdmin", pp_graphs.po2_threshold_min);
}


bool qPrefPartialPressureGas::showPhe() const
{
	return prefs.pp_graphs.phe;
}
void qPrefPartialPressureGas::setShowPhe(bool value)
{
	if (value != prefs.pp_graphs.phe) {
		prefs.pp_graphs.phe = value;
		diskShowPhe(true);
		emit showPheChanged(value);
	}
}
void qPrefPartialPressureGas::diskShowPhe(bool doSync)
{
	LOADSYNC_BOOL("/phegraph", pp_graphs.phe);
}


bool qPrefPartialPressureGas::showPn2() const
{
	return prefs.pp_graphs.pn2;
}
void qPrefPartialPressureGas::setShowPn2(bool value)
{
	if (value != prefs.pp_graphs.pn2) {
		prefs.pp_graphs.pn2 = value;
		diskShowPn2(true);
		emit showPn2Changed(value);
	}
}
void qPrefPartialPressureGas::diskShowPn2(bool doSync)
{
	LOADSYNC_BOOL("/pn2graph", pp_graphs.pn2);
}


bool qPrefPartialPressureGas::showPo2() const
{
	return prefs.pp_graphs.po2;
}
void qPrefPartialPressureGas::setShowPo2(bool value)
{
	if (value != prefs.pp_graphs.po2) {
		prefs.pp_graphs.po2 = value;
		diskShowPo2(true);
		emit showPo2Changed(value);
	}
}
void qPrefPartialPressureGas::diskShowPo2(bool doSync)
{
	LOADSYNC_BOOL("/po2graph", pp_graphs.po2);
}
