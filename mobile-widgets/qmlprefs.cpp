// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"
#include "qmlmanager.h"

#include "core/membuffer.h"
#include "core/gpslocation.h"
#include "core/settings/qPrefUnit.h"


/*** Global and constructors ***/
QMLPrefs *QMLPrefs::m_instance = NULL;

QMLPrefs::QMLPrefs() :
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
