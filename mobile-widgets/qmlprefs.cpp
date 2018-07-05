// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"
#include "qmlmanager.h"

#include "core/membuffer.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include "core/gpslocation.h"


/*** Global and constructors ***/
QMLPrefs *QMLPrefs::m_instance = NULL;

QMLPrefs::QMLPrefs() :
	m_credentialStatus(QMLPrefs::CS_UNKNOWN),
	m_developer(false),
	m_distanceThreshold(1000),
	m_oldStatus(QMLPrefs::CS_UNKNOWN),
	m_showPin(false),
	m_timeThreshold(60)
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

const QString QMLPrefs::cloudUserName() const
{
	return m_cloudUserName;
}

void QMLPrefs::setCloudUserName(const QString &cloudUserName)
{
	m_cloudUserName = cloudUserName.toLower();
	emit cloudUserNameChanged();
}

QMLPrefs::cloud_status QMLPrefs::credentialStatus() const
{
	return m_credentialStatus;
}

void QMLPrefs::setCredentialStatus(const QMLPrefs::cloud_status value)
{
	if (m_credentialStatus != value) {
		setOldStatus(m_credentialStatus);
		if (value == QMLPrefs::CS_NOCLOUD) {
			QMLManager::instance()->appendTextToLog("Switching to no cloud mode");
			set_filename(NOCLOUD_LOCALSTORAGE);
			clearCredentials();
		}
		m_credentialStatus = value;
		emit credentialStatusChanged();
	}
}

void QMLPrefs::setDeveloper(bool value)
{
	m_developer = value;
	emit developerChanged();
}

int QMLPrefs::distanceThreshold() const
{
	return m_distanceThreshold;
}

void QMLPrefs::setDistanceThreshold(int distance)
{
	m_distanceThreshold = distance;
	emit distanceThresholdChanged();
}

QMLPrefs::cloud_status QMLPrefs::oldStatus() const
{
	return m_oldStatus;
}

void QMLPrefs::setOldStatus(const QMLPrefs::cloud_status value)
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

int QMLPrefs::timeThreshold() const
{
	return m_timeThreshold;
}

void QMLPrefs::setTimeThreshold(int time)
{
	m_timeThreshold = time;
	GpsLocation::instance()->setGpsTimeThreshold(m_timeThreshold * 60);
	emit timeThresholdChanged();
}

const QString QMLPrefs::theme() const
{
	QSettings s;
	s.beginGroup("Theme");
	return s.value("currentTheme", "Blue").toString();
}

void QMLPrefs::setTheme(QString theme)
{
	QSettings s;
	s.beginGroup("Theme");
	s.setValue("currentTheme", theme);
	emit themeChanged();
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
	QSettings s;
	
	setCredentialStatus(QMLPrefs::CS_UNKNOWN);
	s.beginGroup("CloudStorage");
	s.setValue("email", m_cloudUserName);
	s.setValue("password", m_cloudPassword);
	s.setValue("cloud_verification_status", m_credentialStatus);
	s.sync();
	QMLManager::instance()->setStartPageText(tr("Starting..."));
	
	setShowPin(false);
}

void QMLPrefs::clearCredentials()
{
	setCloudUserName(NULL);
	setCloudPassword(NULL);
	setCloudPin(NULL);
}
