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
	m_qPrefAnimations(new qPrefAnimations(this)),
	m_qPrefCloudStorage(new qPrefCloudStorage(this)),
	m_qPrefDisplay(new qPrefDisplay(this)),
	m_qPrefDiveComputer(new qPrefDiveComputer(this)),
	m_qPrefDivePlanner(new qPrefDivePlanner(this)),
	m_qPrefFacebook(new qPrefFacebook(this)),
	m_qPrefGeneral(new qPrefGeneral(this)),
	m_qPrefGeocoding(new qPrefGeocoding(this)),
	m_qPrefLanguage(new qPrefLanguage(this)),
	m_qPrefLocationService(new qPrefLocationService(this)),
	m_qPrefPartialPressureGas(new qPrefPartialPressureGas(this)),
	m_qPrefProxy(new qPrefProxy(this)),
	m_qPrefTechnicalDetails(new qPrefTechnicalDetails(this)),
	m_qPrefUnits(new qPrefUnits(this)),
	m_qPrefUpdateManager(new qPrefUpdateManager(this))
{
}


void qPref::loadSync(bool doSync)
{
	m_qPrefAnimations->loadSync(doSync);
	m_qPrefCloudStorage->loadSync(doSync);
	m_qPrefDiveComputer->loadSync(doSync);
	m_qPrefDivePlanner->loadSync(doSync);
	m_qPrefDisplay->loadSync(doSync);
	m_qPrefFacebook->loadSync(doSync);
	m_qPrefGeneral->loadSync(doSync);
	m_qPrefGeocoding->loadSync(doSync);
	m_qPrefLanguage->loadSync(doSync);
	m_qPrefLocationService->loadSync(doSync);
	m_qPrefPartialPressureGas->loadSync(doSync);
	m_qPrefProxy->loadSync(doSync);
	m_qPrefTechnicalDetails->loadSync(doSync);
	m_qPrefUnits->loadSync(doSync);
	m_qPrefUpdateManager->loadSync(doSync);
}
