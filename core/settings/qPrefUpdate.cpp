// SPDX-License-Identifier: GPL-2.0
#include "qPrefUpdate.h"
#include "qPref_private.h"


qPrefUpdate *qPrefUpdate::m_instance = NULL;
qPrefUpdate *qPrefUpdate::instance()
{
	if (!m_instance)
		m_instance = new qPrefUpdate;
	return m_instance;
}


void qPrefUpdate::loadSync(bool doSync)
{
	diskDontCheckForUpdates(doSync);
	diskLastVersionUsed(doSync);
	diskNextCheck(doSync);
}


bool qPrefUpdate::dontCheckExists() const
{
	return prefs.update_manager.dont_check_exists;
}
void qPrefUpdate::setDontCheckExists(bool value)
{
	if (value != prefs.update_manager.dont_check_exists) {
		prefs.update_manager.dont_check_exists = true;
		emit dontCheckForExistsChanged(value);
		// Do not save on disk !!
	}
}


bool qPrefUpdate::dontCheckForUpdates() const
{
	return prefs.update_manager.dont_check_for_updates;
}
void qPrefUpdate::setDontCheckForUpdates(bool value)
{
	if (value != prefs.update_manager.dont_check_for_updates) {
		prefs.update_manager.dont_check_for_updates = value;
		prefs.update_manager.dont_check_exists = true;
		diskDontCheckForUpdates(true);
		emit dontCheckForUpdatesChanged(value);
	}
}
void qPrefUpdate::diskDontCheckForUpdates(bool doSync)
{
	LOADSYNC_BOOL("/DontCheckForUpdates", update_manager.dont_check_for_updates);
}


const QString qPrefUpdate::lastVersionUsed() const
{
	return prefs.update_manager.last_version_used;
}
void qPrefUpdate::setLastVersionUsed(const QString& value)
{
	if (value != prefs.update_manager.last_version_used) {
		COPY_TXT(update_manager.last_version_used, value);
		diskLastVersionUsed(true);
		emit lastVersionUsedChanged(value);
	}
}
void qPrefUpdate::diskLastVersionUsed(bool doSync)
{
	LOADSYNC_TXT("/LastVersionUsed", update_manager.last_version_used);
}


const QDate qPrefUpdate::nextCheck() const
{
	return QDate::fromString(QString(prefs.update_manager.next_check), "dd/MM/yyyy");
}
void qPrefUpdate::setNextCheck(const QDate& value)
{
	if (value.toString() != prefs.update_manager.next_check) {
		COPY_TXT(update_manager.next_check, value.toString("dd/MM/yyyy"));
		diskNextCheck(true);
		emit nextCheckChanged(value);
	}
}
void qPrefUpdate::diskNextCheck(bool doSync)
{
	LOADSYNC_TXT("/NextCheck", update_manager.next_check);
}
