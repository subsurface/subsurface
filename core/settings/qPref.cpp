// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"

#include "../membuffer.h"
#include "../subsurface-qt/SettingsObjectWrapper.h"
#include "../gpslocation.h"


/*** Global and constructors ***/
qPref *qPref::m_instance = NULL;

qPref::qPref() :
	m_credentialStatus(CS_UNKNOWN),
	m_developer(false),
	m_distanceThreshold(1000),
	m_oldStatus(CS_UNKNOWN),
	m_showPin(false),
	m_timeThreshold(60)
{
	// This strange construct is needed because QMLEngine calls new and that
	// cannot be overwritten
	if (!m_instance)
		m_instance = this;
}

qPref::~qPref()
{
	m_instance = NULL;
}

qPref *qPref::instance()
{
	return m_instance;
}


/*** public functions ***/
const QString qPref::cloudPassword() const
{
	return m_cloudPassword;
}

void qPref::setCloudPassword(const QString &cloudPassword)
{
	m_cloudPassword = cloudPassword;
	emit cloudPasswordChanged();
}

const QString qPref::cloudPin() const
{
	return m_cloudPin;
}

void qPref::setCloudPin(const QString &cloudPin)
{
	m_cloudPin = cloudPin;
	emit cloudPinChanged();
}

const QString qPref::cloudUserName() const
{
	return m_cloudUserName;
}

void qPref::setCloudUserName(const QString &cloudUserName)
{
	m_cloudUserName = cloudUserName.toLower();
	emit cloudUserNameChanged();
}

qPref::cloud_status_qml qPref::credentialStatus() const
{
	return m_credentialStatus;
}

void qPref::setCredentialStatus(const cloud_status_qml value)
{
	if (m_credentialStatus != value) {
		setOldStatus(m_credentialStatus);
		if (value == CS_NOCLOUD) {
			qDebug() << "Switching to no cloud mode";
			set_filename(NOCLOUD_LOCALSTORAGE);
			clearCredentials();
		}
		m_credentialStatus = value;
		emit credentialStatusChanged();
	}
}

void qPref::setDeveloper(bool value)
{
	m_developer = value;
	emit developerChanged();
}

int qPref::distanceThreshold() const
{
	return m_distanceThreshold;
}

void qPref::setDistanceThreshold(int distance)
{
	m_distanceThreshold = distance;
	emit distanceThresholdChanged();
}

qPref::cloud_status_qml qPref::oldStatus() const
{
	return m_oldStatus;
}

void qPref::setOldStatus(const cloud_status_qml value)
{
	if (m_oldStatus != value) {
		m_oldStatus = value;
		emit oldStatusChanged();
	}
}

bool qPref::showPin() const
{
	return m_showPin;
}

void qPref::setShowPin(bool enable)
{
	m_showPin = enable;
	emit showPinChanged();
}

int qPref::timeThreshold() const
{
	return m_timeThreshold;
}

void qPref::setTimeThreshold(int time)
{
	m_timeThreshold = time;
	GpsLocation::instance()->setGpsTimeThreshold(m_timeThreshold * 60);
	emit timeThresholdChanged();
}

const QString qPref::theme() const
{
	QSettings s;
	s.beginGroup("Theme");
	return s.value("currentTheme", "Blue").toString();
}

void qPref::setTheme(QString theme)
{
	QSettings s;
	s.beginGroup("Theme");
	s.setValue("currentTheme", theme);
	emit themeChanged();
}



/*** public slot functions ***/
void qPref::cancelCredentialsPinSetup()
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
	
	setCredentialStatus(CS_UNKNOWN);
	s.beginGroup("CloudStorage");
	s.setValue("email", m_cloudUserName);
	s.setValue("password", m_cloudPassword);
	s.setValue("cloud_verification_status", m_credentialStatus);
	s.sync();
//FIX for now	QMLManager::instance()->setStartPageText(tr("Starting..."));
	
	setShowPin(false);
}

void qPref::clearCredentials()
{
	setCloudUserName(NULL);
	setCloudPassword(NULL);
	setCloudPin(NULL);
}
