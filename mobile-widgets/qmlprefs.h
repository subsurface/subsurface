// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPREFS_H
#define QMLPREFS_H

#include <QObject>
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDisplay.h"


class QMLPrefs : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString cloudPin
				MEMBER m_cloudPin
				WRITE setCloudPin
				NOTIFY cloudPinChanged)
	Q_PROPERTY(qPrefCloudStorage::cloud_status credentialStatus
				MEMBER m_credentialStatus
				WRITE setCredentialStatus
				NOTIFY credentialStatusChanged)
	Q_PROPERTY(bool showPin
				MEMBER m_showPin
				WRITE setShowPin
				NOTIFY showPinChanged)
	Q_PROPERTY(qPrefCloudStorage::cloud_status oldStatus
				MEMBER m_oldStatus
				WRITE setOldStatus
				NOTIFY oldStatusChanged)
public:
	QMLPrefs();
	~QMLPrefs();

	static QMLPrefs *instance();

	const QString cloudPin() const;
	void setCloudPin(const QString &cloudPin);

	qPrefCloudStorage::cloud_status credentialStatus() const;
	void setCredentialStatus(const qPrefCloudStorage::cloud_status value);

	qPrefCloudStorage::cloud_status oldStatus() const;
	void setOldStatus(const qPrefCloudStorage::cloud_status value);

	bool showPin() const;
	void setShowPin(bool enable);

public slots:
	void cancelCredentialsPinSetup();
	void clearCredentials();

private:
	QString m_cloudPin;
	qPrefCloudStorage::cloud_status m_credentialStatus;
	static QMLPrefs *m_instance;
	qPrefCloudStorage::cloud_status m_oldStatus;
	bool m_showPin;

signals:
	void cloudPinChanged();
	void credentialStatusChanged();
	void oldStatusChanged();
	void showPinChanged();
};

#endif
