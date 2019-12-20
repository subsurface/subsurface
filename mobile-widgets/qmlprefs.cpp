// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"
#include "qmlmanager.h"

#include "core/membuffer.h"
#include "core/gpslocation.h"
#include "core/settings/qPrefUnit.h"


/*** Global and constructors ***/
QMLPrefs *QMLPrefs::m_instance = NULL;

QMLPrefs::QMLPrefs() :
	m_credentialStatus(qPrefCloudStorage::CS_UNKNOWN),
	m_oldStatus(qPrefCloudStorage::CS_UNKNOWN),
	m_showPin(false)
{
	// This strange construct is needed because QMLEngine calls new and that
	// cannot be overwritten
	if (!m_instance)
		m_instance = this;
}

QMLPrefs::~QMLPrefs()
{
	m_instance = NULL;
}

QMLPrefs *QMLPrefs::instance()
{
	return m_instance;
}


/*** public functions ***/
const QString QMLPrefs::cloudPassword() const
{
	return m_cloudPassword;
}

void QMLPrefs::setCloudPassword(const QString &cloudPassword)
{
	m_cloudPassword = cloudPassword;
	emit cloudPasswordChanged();
}

const QString QMLPrefs::cloudPin() const
{
	return m_cloudPin;
}

void QMLPrefs::setCloudPin(const QString &cloudPin)
{
	m_cloudPin = cloudPin;
	emit cloudPinChanged();
}

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
			clearCredentials();
			if (qPrefUnits::unit_system() == "imperial")
				prefs.units = IMPERIAL_units;
			else if (qPrefUnits::unit_system() == "metric")
				prefs.units = SI_units;
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

/*** public slot functions ***/
void QMLPrefs::cancelCredentialsPinSetup()
{   
	/* 
	 * The user selected <cancel> on the final stage of the
	 * cloud account generation (entering the emailed PIN).
	 * 
	 * Resets the cloud credential status to CS_UNKNOWN, resulting
	 * in a return to the first crededentials page, with the
	 * email and passwd still filled in. In case of a cancel
	 * of registration (from the PIN page), the email address
	 * was probably misspelled, so the user never received a PIN to
	 * complete the process.
	 * 
	 * Notice that this function is also used to switch to a different
	 * cloud account, so the name is not perfect.
	 */
	
	setCredentialStatus(qPrefCloudStorage::CS_UNKNOWN);
	qPrefCloudStorage::set_cloud_verification_status(m_credentialStatus);
	QMLManager::instance()->setStartPageText(tr("Starting..."));
	
	setShowPin(false);
}

void QMLPrefs::clearCredentials()
{
	qPrefCloudStorage::set_cloud_storage_email(NULL);
	setCloudPassword(NULL);
	setCloudPin(NULL);
}
