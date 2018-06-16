// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H

#include "qPrefAnimations.h"

#include <QObject>


class qPref : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status_qml)
	Q_PROPERTY(QString cloudPassword
				MEMBER m_cloudPassword
				WRITE setCloudPassword
				NOTIFY cloudPasswordChanged)
	Q_PROPERTY(QString cloudPin
				MEMBER m_cloudPin
				WRITE setCloudPin
				NOTIFY cloudPinChanged)
	Q_PROPERTY(QString cloudUserName
				MEMBER m_cloudUserName
				WRITE setCloudUserName
				NOTIFY cloudUserNameChanged)
	Q_PROPERTY(cloud_status_qml credentialStatus
				MEMBER m_credentialStatus
				WRITE setCredentialStatus
				NOTIFY credentialStatusChanged)
	Q_PROPERTY(bool developer
				MEMBER m_developer
				WRITE setDeveloper
				NOTIFY developerChanged)
	Q_PROPERTY(int distanceThreshold
				MEMBER m_distanceThreshold
				WRITE setDistanceThreshold
				NOTIFY distanceThresholdChanged)
	Q_PROPERTY(bool showPin
				MEMBER m_showPin
				WRITE setShowPin
				NOTIFY showPinChanged)
	Q_PROPERTY(cloud_status_qml oldStatus
				MEMBER m_oldStatus
				WRITE setOldStatus
				NOTIFY oldStatusChanged)
	Q_PROPERTY(QString theme
				READ theme
				WRITE setTheme
				NOTIFY themeChanged)
	Q_PROPERTY(int timeThreshold
				MEMBER m_timeThreshold
				WRITE setTimeThreshold
				NOTIFY timeThresholdChanged)

public:
	qPref();
	~qPref();

	static qPref *instance();

	enum cloud_status_qml {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};

	const QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	const QString cloudPin() const;
	void setCloudPin(const QString &cloudPin);

	const QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	cloud_status_qml credentialStatus() const;
	void setCredentialStatus(const cloud_status_qml value);

	void setDeveloper(bool value);

	int distanceThreshold() const;
	void setDistanceThreshold(int distance);

	cloud_status_qml oldStatus() const;
	void setOldStatus(const cloud_status_qml value);

	bool showPin() const;
	void setShowPin(bool enable);

	int		timeThreshold() const;
	void	setTimeThreshold(int time);

	const QString theme() const;
	void setTheme(QString theme);

public slots:
	void cancelCredentialsPinSetup();
	void clearCredentials();

private:
	QString m_cloudPassword;
	QString m_cloudPin;
	QString m_cloudUserName;
	cloud_status_qml m_credentialStatus;
	bool m_developer;
	int m_distanceThreshold;
	static qPref *m_instance;
	cloud_status_qml m_oldStatus;
	bool m_showPin;
	int m_timeThreshold;

signals:
	void cloudPasswordChanged();
	void cloudPinChanged();
	void cloudUserNameChanged();
	void credentialStatusChanged();
	void distanceThresholdChanged();
	void developerChanged();
	void oldStatusChanged();
	void showPinChanged();
	void themeChanged();
	void timeThresholdChanged();
};

#define NOCLOUD_LOCALSTORAGE format_string("%s/cloudstorage/localrepo[master]", system_default_directory())

#endif
