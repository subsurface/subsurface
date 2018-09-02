// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"

qPref::qPref(QObject *parent) : QObject(parent)
{
}

qPref *qPref::instance()
{
	static qPref *self = new qPref;
	return self;
}

void qPref::loadSync(bool doSync)
{
	if (!doSync)
		uiLanguage(NULL);

	qPrefCloudStorage::instance()->loadSync(doSync);
	qPrefDisplay::instance()->loadSync(doSync);
	qPrefDiveComputer::instance()->loadSync(doSync);
	qPrefDivePlanner::instance()->loadSync(doSync);
	// qPrefFaceook does not use disk.
	qPrefGeneral::instance()->loadSync(doSync);
	qPrefGeocoding::instance()->loadSync(doSync);
	qPrefLanguage::instance()->loadSync(doSync);
	qPrefLocationService::instance()->loadSync(doSync);
	qPrefPartialPressureGas::instance()->loadSync(doSync);
	qPrefProxy::instance()->loadSync(doSync);
	qPrefTechnicalDetails::instance()->loadSync(doSync);
	qPrefUnits::instance()->loadSync(doSync);
	qPrefUpdateManager::instance()->loadSync(doSync);
}
