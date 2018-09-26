// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"
#include "qmlmanager.h"

#include "core/membuffer.h"
#include "core/gpslocation.h"
#include "core/settings/qPrefUnit.h"


/*** Global and constructors ***/
QMLPrefs::QMLPrefs()
{
}

QMLPrefs *QMLPrefs::instance()
{
	static QMLPrefs *self = new QMLPrefs;
	return self;
}


/*** public functions ***/
qPrefCloudStorage::cloud_status QMLPrefs::credentialStatus() const
{
	return (qPrefCloudStorage::cloud_status)qPrefCloudStorage::cloud_verification_status();
}

void QMLPrefs::setCredentialStatus(const qPrefCloudStorage::cloud_status value)
{
	qPrefCloudStorage::set_cloud_verification_status(value);
	emit credentialStatusChanged();
}
