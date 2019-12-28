// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPREFS_H
#define QMLPREFS_H

#include <QObject>
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDisplay.h"


class QMLPrefs : public QObject {
	Q_OBJECT
	Q_PROPERTY(qPrefCloudStorage::cloud_status credentialStatus
				READ credentialStatus
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

	qPrefCloudStorage::cloud_status credentialStatus() const;
	void setCredentialStatus(const qPrefCloudStorage::cloud_status value);

	qPrefCloudStorage::cloud_status oldStatus() const;
	void setOldStatus(const qPrefCloudStorage::cloud_status value);

	bool showPin() const;
	void setShowPin(bool enable);

private:
	static QMLPrefs *m_instance;
	qPrefCloudStorage::cloud_status m_oldStatus;
	bool m_showPin;

signals:
	void credentialStatusChanged();
	void oldStatusChanged();
	void showPinChanged();
};

#endif
