// SPDX-License-Identifier: GPL-2.0
#include "qPrefLocationService.h"
#include "qPref_private.h"



qPrefLocationService *qPrefLocationService::m_instance = NULL;
qPrefLocationService *qPrefLocationService::instance()
{
	if (!m_instance)
		m_instance = new qPrefLocationService;
	return m_instance;
}



void qPrefLocationService::loadSync(bool doSync)
{
	diskDistanceThreshold(doSync);
	diskTimeThreshold(doSync);
}


int qPrefLocationService::distanceThreshold() const
{
	return prefs.distance_threshold;
}
void qPrefLocationService::setDistanceThreshold(int value)
{
	if (value != prefs.distance_threshold) {
		prefs.distance_threshold = value;
		diskDistanceThreshold(true);
		emit distanceThresholdChanged(value);
	}
}
void qPrefLocationService::diskDistanceThreshold(bool doSync)
{
	LOADSYNC_INT("/distance_threshold", distance_threshold);
}


int qPrefLocationService::timeThreshold() const
{
	return prefs.time_threshold;
}
void qPrefLocationService::setTimeThreshold(int value)
{
	if (value != prefs.time_threshold) {
		prefs.time_threshold = value;
		diskTimeThreshold(true);
		emit timeThresholdChanged(value);
	}
}
void qPrefLocationService::diskTimeThreshold(bool doSync)
{
	LOADSYNC_INT("/time_threshold", time_threshold);
}
