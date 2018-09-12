// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPREFS_H
#define QMLPREFS_H

#include <QObject>
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDisplay.h"


class QMLPrefs : public QObject {
	Q_OBJECT
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
	Q_PROPERTY(qPrefCloudStorage::cloud_status credentialStatus
				MEMBER m_credentialStatus
				WRITE setCredentialStatus
				NOTIFY credentialStatusChanged)
	Q_PROPERTY(int distanceThreshold
				MEMBER m_distanceThreshold
				WRITE setDistanceThreshold
				NOTIFY distanceThresholdChanged)
	Q_PROPERTY(bool showPin
				MEMBER m_showPin
				WRITE setShowPin
				NOTIFY showPinChanged)
	Q_PROPERTY(qPrefCloudStorage::cloud_status oldStatus
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
	QMLPrefs();
	~QMLPrefs();

	static QMLPrefs *instance();

	const QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	const QString cloudPin() const;
	void setCloudPin(const QString &cloudPin);

	const QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	qPrefCloudStorage::cloud_status credentialStatus() const;
	void setCredentialStatus(const qPrefCloudStorage::cloud_status value);

	int distanceThreshold() const;
	void setDistanceThreshold(int distance);

	qPrefCloudStorage::cloud_status oldStatus() const;
	void setOldStatus(const qPrefCloudStorage::cloud_status value);

	bool showPin() const;
	void setShowPin(bool enable);

	int  timeThreshold() const;
	void setTimeThreshold(int time);

	const QString theme() const;
	void setTheme(QString theme);

public slots:
	void cancelCredentialsPinSetup();
	void clearCredentials();

private:
	QString m_cloudPassword;
	QString m_cloudPin;
	QString m_cloudUserName;
	qPrefCloudStorage::cloud_status m_credentialStatus;
	int m_distanceThreshold;
	static QMLPrefs *m_instance;
	qPrefCloudStorage::cloud_status m_oldStatus;
	bool m_showPin;
	int m_timeThreshold;

signals:
	void cloudPasswordChanged();
	void cloudPinChanged();
	void cloudUserNameChanged();
	void credentialStatusChanged();
	void distanceThresholdChanged();
	void oldStatusChanged();
	void showPinChanged();
	void themeChanged();
	void timeThresholdChanged();
};

#endif
