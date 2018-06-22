// SPDX-License-Identifier: GPL-2.0
#include "qPrefGas.h"
#include "qPref_private.h"



qPrefGas *qPrefGas::m_instance = NULL;
qPrefGas *qPrefGas::instance()
{
	if (!m_instance)
		m_instance = new qPrefGas;
	return m_instance;
}

void qPrefGas::loadSync(bool doSync)
{
	diskPheThreshold(doSync);
	diskPn2Threshold(doSync);
	diskPo2ThresholdMin(doSync);
	diskPo2ThresholdMax(doSync);
	diskShowPhe(doSync);
	diskShowPn2(doSync);
	diskShowPo2(doSync);
}


double qPrefGas::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}
void qPrefGas::setPheThreshold(double value)
{
	if (value != prefs.pp_graphs.phe_threshold) {
		prefs.pp_graphs.phe_threshold = value;
		diskPheThreshold(true);
		emit pheThresholdChanged(value);
	}
}
void qPrefGas::diskPheThreshold(bool doSync)
{
	LOADSYNC_DOUBLE("/phethreshold", pp_graphs.phe_threshold);
}


double qPrefGas::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}
void qPrefGas::setPn2Threshold(double value)
{
	if (value != prefs.pp_graphs.pn2_threshold) {
		prefs.pp_graphs.pn2_threshold = value;
		diskPn2Threshold(true);
		emit pn2ThresholdChanged(value);
	}
}
void qPrefGas::diskPn2Threshold(bool doSync)
{
	LOADSYNC_DOUBLE("/pn2threshold", pp_graphs.pn2_threshold);
}


double qPrefGas::po2ThresholdMax() const
{
	return prefs.pp_graphs.po2_threshold_max;
}
void qPrefGas::setPo2ThresholdMax(double value)
{
	if (value != prefs.pp_graphs.po2_threshold_max) {
		prefs.pp_graphs.po2_threshold_max = value;
		diskPo2ThresholdMax(true);
		emit po2ThresholdMaxChanged(value);
	}
}
void qPrefGas::diskPo2ThresholdMax(bool doSync)
{
	LOADSYNC_DOUBLE("/po2thresholdmax", pp_graphs.po2_threshold_max);
}


double qPrefGas::po2ThresholdMin() const
{
	return prefs.pp_graphs.po2_threshold_min;
}
void qPrefGas::setPo2ThresholdMin(double value)
{
	if (value != prefs.pp_graphs.po2_threshold_min) {
		prefs.pp_graphs.po2_threshold_min = value;
		emit po2ThresholdMinChanged(value);
	}
}
void qPrefGas::diskPo2ThresholdMin(bool doSync)
{
	LOADSYNC_DOUBLE("/po2thresholdmin", pp_graphs.po2_threshold_min);
}


bool qPrefGas::showPhe() const
{
	return prefs.pp_graphs.phe;
}
void qPrefGas::setShowPhe(bool value)
{
	if (value != prefs.pp_graphs.phe) {
		prefs.pp_graphs.phe = value;
		diskShowPhe(true);
		emit showPheChanged(value);
	}
}
void qPrefGas::diskShowPhe(bool doSync)
{
	LOADSYNC_BOOL("/phegraph", pp_graphs.phe);
}


bool qPrefGas::showPn2() const
{
	return prefs.pp_graphs.pn2;
}
void qPrefGas::setShowPn2(bool value)
{
	if (value != prefs.pp_graphs.pn2) {
		prefs.pp_graphs.pn2 = value;
		diskShowPn2(true);
		emit showPn2Changed(value);
	}
}
void qPrefGas::diskShowPn2(bool doSync)
{
	LOADSYNC_BOOL("/pn2graph", pp_graphs.pn2);
}


bool qPrefGas::showPo2() const
{
	return prefs.pp_graphs.po2;
}
void qPrefGas::setShowPo2(bool value)
{
	if (value != prefs.pp_graphs.po2) {
		prefs.pp_graphs.po2 = value;
		diskShowPo2(true);
		emit showPo2Changed(value);
	}
}
void qPrefGas::diskShowPo2(bool doSync)
{
	LOADSYNC_BOOL("/po2graph", pp_graphs.po2);
}
