// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"
#include "qmlmanager.h"

#include "core/membuffer.h"
#include "core/gpslocation.h"
#include "core/settings/qPrefUnit.h"


/*** Global and constructors ***/
QMLPrefs::QMLPrefs() :
	m_showPin(false)
{
	m_credentialStatus = (qPrefCloudStorage::cloud_status) qPrefCloudStorage::cloud_verification_status();
	m_oldStatus = m_credentialStatus;
}

QMLPrefs *QMLPrefs::instance()
{
	static QMLPrefs *self = new QMLPrefs;
	return self;
}


/*** public functions ***/
qPrefCloudStorage::cloud_status QMLPrefs::credentialStatus() const
{
	return m_credentialStatus;
}

void QMLPrefs::setCredentialStatus(const qPrefCloudStorage::cloud_status value)
{
	if (m_credentialStatus != value) {
		setOldStatus(m_credentialStatus);
		if (value == qPrefCloudStorage::CS_NOCLOUD) {
			QMLManager::instance()->appendTextToLog("Switching to no cloud mode");
			set_filename(NOCLOUD_LOCALSTORAGE);
			if (qPrefUnits::unit_system() == "imperial")
				prefs.units = IMPERIAL_units;
			else if (qPrefUnits::unit_system() == "metric")
				prefs.units = SI_units;
			// do not clear cloud user to let user remember from where the data came
			qPrefCloudStorage::set_cloud_storage_password("");
		}
		m_credentialStatus = value;
		emit credentialStatusChanged();
	}
}

qPrefCloudStorage::cloud_status QMLPrefs::oldStatus() const
{
	return m_oldStatus;
}

void QMLPrefs::setOldStatus(const qPrefCloudStorage::cloud_status value)
{
	if (m_oldStatus != value) {
		m_oldStatus = value;
		emit oldStatusChanged();
	}
}

bool QMLPrefs::showPin() const
{
	return m_showPin;
}

void QMLPrefs::setShowPin(bool enable)
{
	m_showPin = enable;
	emit showPinChanged();
}
