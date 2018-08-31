// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"

#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>

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

	// the following calls, ensures qPref* is instanciated, registred and
	// that properties are loaded
	qPrefCloudStorage::loadSync(doSync);
	qPrefDisplay::loadSync(doSync);
	qPrefDiveComputer::loadSync(doSync);
	qPrefDivePlanner::loadSync(doSync);
	qPrefFacebook::loadSync(doSync);
	qPrefGeneral::loadSync(doSync);
	qPrefGeocoding::loadSync(doSync);
	qPrefLanguage::loadSync(doSync);
	qPrefLocationService::loadSync(doSync);
	qPrefPartialPressureGas::loadSync(doSync);
	qPrefProxy::loadSync(doSync);
	qPrefTechnicalDetails::loadSync(doSync);
	qPrefUnits::loadSync(doSync);
	qPrefUpdateManager::loadSync(doSync);
}

#define REGISTER_QPREF(useClass, useQML) \
	rc = qmlRegisterType<useClass>("org.subsurfacedivelog.mobile", 1, 0, useQML); \
	if (rc < 0) \
		qWarning() << "ERROR: Cannot register " << useQML << ", QML will not work!!";

void qPref::registerQML()
{
	int rc;

	REGISTER_QPREF(qPref, "SsrfPrefs");
	REGISTER_QPREF(qPrefCloudStorage, "SsrfCloudStoragePrefs");
	REGISTER_QPREF(qPrefDisplay, "SsrfDisplayPrefs");
	REGISTER_QPREF(qPrefDiveComputer, "SsrfDiveComputerPrefs");
	REGISTER_QPREF(qPrefDivePlanner, "SsrfDivePlannerPrefs");
	REGISTER_QPREF(qPrefFacebook, "SsrfFacebookPrefs");
	REGISTER_QPREF(qPrefGeneral, "SsrfGeneralPrefs");
	REGISTER_QPREF(qPrefGeocoding, "SsrfGeocodingPrefs");
	REGISTER_QPREF(qPrefLanguage, "SsrfLanguagePrefs");
	REGISTER_QPREF(qPrefLocationService, "SsrfLocationServicePrefs");
	REGISTER_QPREF(qPrefPartialPressureGas, "SsrfPartialPressureGasPrefs");
	REGISTER_QPREF(qPrefProxy, "SsrfProxyPrefs");
	REGISTER_QPREF(qPrefTechnicalDetails, "SsrfTechnicalDetailsPrefs");
	REGISTER_QPREF(qPrefUnits, "SsrfUnitPrefs");
	REGISTER_QPREF(qPrefUpdateManager, "SsrfUpdateManagerPrefs");
}
