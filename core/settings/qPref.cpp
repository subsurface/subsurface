// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"
#include "ssrf-version.h"

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
	qPrefAnimations::instance()->loadSync(doSync);
	qPrefCloudStorage::instance()->loadSync(doSync);
	qPrefDisplay::instance()->loadSync(doSync);
	qPrefDiveComputer::instance()->loadSync(doSync);
	qPrefDivePlanner::instance()->loadSync(doSync);
	// qPrefFaceook does not use disk.
	qPrefGeocoding::instance()->loadSync(doSync);
	qPrefLanguage::instance()->loadSync(doSync);
	qPrefLocationService::instance()->loadSync(doSync);
	qPrefPartialPressureGas::instance()->loadSync(doSync);
	qPrefProxy::instance()->loadSync(doSync);
	qPrefTechnicalDetails::instance()->loadSync(doSync);
	qPrefUnits::instance()->loadSync(doSync);
	qPrefUpdateManager::instance()->loadSync(doSync);
}

const QString qPref::canonical_version() const
{
	return QString(CANONICAL_VERSION_STRING);
}

const QString qPref::mobile_version() const
{
	return QString(MOBILE_VERSION_STRING);
}
