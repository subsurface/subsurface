// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPref.h"


qPref *qPref::m_instance = NULL;
qPref *qPref::instance()
{
	if (!m_instance)
		m_instance = new qPref;
	return m_instance;
}
qPref::qPref(QObject *parent) :
	QObject(parent),
	animations(new qPrefAnimations(this)),
	cloudStorage(new qPrefCloudStorage(this)),
	display(new qPrefDisplay(this)),
	diveComputer(new qPrefDiveComputer(this)),
	divePlanner(new qPrefDivePlanner(this)),
	facebook(new qPrefFacebook(this)),
	general(new qPrefGeneral(this)),
	geocoding(new qPrefGeocoding(this)),
	language(new qPrefLanguage(this)),
	locationService(new qPrefLocationService(this)),
	partialPressureGas(new qPrefPartialPressureGas(this)),
	proxy(new qPrefProxy(this)),
	technicalDetails(new qPrefTechnicalDetails(this)),
	units(new qPrefUnits(this)),
	updateManager(new qPrefUpdateManager(this))
{
}


void qPref::loadSync(bool doSync)
{
	animations->loadSync(doSync);
	cloudStorage->loadSync(doSync);
	diveComputer->loadSync(doSync);
	divePlanner->loadSync(doSync);
	display->loadSync(doSync);
	facebook->loadSync(doSync);
	general->loadSync(doSync);
	geocoding->loadSync(doSync);
	language->loadSync(doSync);
	locationService->loadSync(doSync);
	partialPressureGas->loadSync(doSync);
	proxy->loadSync(doSync);
	technicalDetails->loadSync(doSync);
	units->loadSync(doSync);
	updateManager->loadSync(doSync);
}
