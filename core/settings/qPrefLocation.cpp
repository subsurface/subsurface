// SPDX-License-Identifier: GPL-2.0
#include "qPrefLocation.h"
#include "qPref_private.h"



qPrefLocation *qPrefLocation::m_instance = NULL;
qPrefLocation *qPrefLocation::instance()
{
	if (!m_instance)
		m_instance = new qPrefLocation;
	return m_instance;
}



void qPrefLocation::loadSync(bool doSync)
{
	diskDistanceThreshold(doSync);
	diskTimeThreshold(doSync);
}


int qPrefLocation::distanceThreshold() const
{
	return prefs.distance_threshold;
}
void qPrefLocation::setDistanceThreshold(int value)
{
	if (value != prefs.distance_threshold) {
		prefs.distance_threshold = value;
		diskDistanceThreshold(true);
		emit distanceThresholdChanged(value);
	}
}
void qPrefLocation::diskDistanceThreshold(bool doSync)
{
	LOADSYNC_INT("/distance_threshold", distance_threshold);
}


int qPrefLocation::timeThreshold() const
{
	return prefs.time_threshold;
}
void qPrefLocation::setTimeThreshold(int value)
{
	if (value != prefs.time_threshold) {
		prefs.time_threshold = value;
		diskTimeThreshold(true);
		emit timeThresholdChanged(value);
	}
}
void qPrefLocation::diskTimeThreshold(bool doSync)
{
	LOADSYNC_INT("/time_threshold", time_threshold);
}
