
#include "qPrefUpdateManager.h"
#include "qPref_private.h"



qPrefUpdateManager *qPrefUpdateManager::m_instance = NULL;
qPrefUpdateManager *qPrefUpdateManager::instance()
{
	if (!m_instance)
		m_instance = new qPrefUpdateManager;
	return m_instance;
}


void qPrefUpdateManager::loadSync(bool doSync)
{
	diskDontCheckForUpdates(doSync);
	diskLastVersionUsed(doSync);
	diskNextCheck(doSync);
}


bool qPrefUpdateManager::dontCheckForUpdates() const
{
	return prefs.update_manager.dont_check_for_updates;
}
void qPrefUpdateManager::setDontCheckForUpdates(bool value)
{
	if (value != prefs.update_manager.dont_check_for_updates) {
		prefs.update_manager.dont_check_for_updates = value;
		diskDontCheckForUpdates(true);
		emit dontCheckForUpdatesChanged(value);
	}
}
void qPrefUpdateManager::diskDontCheckForUpdates(bool doSync)
{
	LOADSYNC_BOOL("DontCheckForUpdates", update_manager.dont_check_for_updates);
}


bool qPrefUpdateManager::dontCheckExists() const
{
	return prefs.update_manager.dont_check_exists;
}
void qPrefUpdateManager::setDontCheckExists(bool value)
{
	if (value != prefs.update_manager.dont_check_for_updates) {
		prefs.update_manager.dont_check_exists = value;
		emit dontCheckExistsChanged(value);
	}
	// DO NOT STORE ON DISK
}


const QString qPrefUpdateManager::lastVersionUsed() const
{
	return prefs.update_manager.last_version_used;
}
void qPrefUpdateManager::setLastVersionUsed(const QString& value)
{
	if (value != prefs.update_manager.last_version_used) {
		COPY_TXT(update_manager.last_version_used, value);
		diskLastVersionUsed(true);
		emit lastVersionUsedChanged(value);
	}
}
void qPrefUpdateManager::diskLastVersionUsed(bool doSync)
{
	LOADSYNC_TXT("LastVersionUsed", update_manager.last_version_used);
}


const QDate qPrefUpdateManager::nextCheck() const
{
	return QDate::fromString(QString(prefs.update_manager.next_check), "dd/MM/yyyy");
}
void qPrefUpdateManager::setNextCheck(const QDate& value)
{
	QString valueString = value.toString("dd/MM/yyyy");
	if (valueString != prefs.update_manager.next_check) {
		COPY_TXT(update_manager.next_check, valueString);
		diskNextCheck(true);
		emit nextCheckChanged(value);
	}
}
void qPrefUpdateManager::diskNextCheck(bool doSync)
{
	LOADSYNC_TXT("NextCheck", update_manager.next_check);
}

