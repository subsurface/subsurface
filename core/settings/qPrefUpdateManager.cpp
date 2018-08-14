// SPDX-License-Identifier: GPL-2.0
#include "qPrefUpdateManager.h"
#include "qPrefPrivate.h"


static const QString group = QStringLiteral("UpdateManager");

qPrefUpdateManager::qPrefUpdateManager(QObject *parent) : QObject(parent)
{
}

qPrefUpdateManager *qPrefUpdateManager::instance()
{
	static qPrefUpdateManager *self = new qPrefUpdateManager;
	return self;
}


void qPrefUpdateManager::loadSync(bool doSync)
{
	disk_dont_check_for_updates(doSync);
	disk_last_version_used(doSync);
	disk_next_check(doSync);
}


HANDLE_PREFERENCE_BOOL_EXT(UpdateManager, "/DontCheckForUpdates", dont_check_for_updates, update_manager.);

void qPrefUpdateManager::set_dont_check_exists(bool value)
{
	if (value != prefs.update_manager.dont_check_exists) {
		prefs.update_manager.dont_check_exists = value;
		emit instance()->dont_check_exists_changed(value);
	}
	// DO NOT STORE ON DISK
}


HANDLE_PREFERENCE_TXT_EXT(UpdateManager, "/LastVersionUsed", last_version_used, update_manager.);


void qPrefUpdateManager::set_next_check(const QDate& value)
{
	QString valueString = value.toString("dd/MM/yyyy");
	if (valueString != prefs.update_manager.next_check) {
		qPrefPrivate::copy_txt(&prefs.update_manager.next_check, valueString);
		disk_next_check(true);
		emit instance()->next_check_changed(value);
	}
}
DISK_LOADSYNC_TXT_EXT(UpdateManager, "/NextCheck", next_check, update_manager.);

