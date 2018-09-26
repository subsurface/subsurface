// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPREFS_H
#define QMLPREFS_H

#include <QObject>
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDisplay.h"


class QMLPrefs : public QObject {
	Q_OBJECT
	Q_PROPERTY(qPrefCloudStorage::cloud_status credentialStatus
				MEMBER m_credentialStatus
				WRITE setCredentialStatus
				NOTIFY credentialStatusChanged)
public:
	static QMLPrefs *instance();

	qPrefCloudStorage::cloud_status credentialStatus() const;
	void setCredentialStatus(const qPrefCloudStorage::cloud_status value);

public slots:

private:
	QMLPrefs();

	qPrefCloudStorage::cloud_status m_credentialStatus;

signals:
	void credentialStatusChanged();
};

#endif
