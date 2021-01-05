// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUPDATEMANAGER_H
#define QPREFUPDATEMANAGER_H
#include "core/pref.h"

#include <QObject>
#include <QDate>

class qPrefUpdateManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dont_check_for_updates READ dont_check_for_updates WRITE set_dont_check_for_updates NOTIFY dont_check_for_updatesChanged)
	Q_PROPERTY(const QString last_version_used READ last_version_used WRITE set_last_version_used NOTIFY last_version_usedChanged)
	Q_PROPERTY(const QDate next_check READ next_check WRITE set_next_check NOTIFY next_checkChanged)
	Q_PROPERTY(const QString uuidString READ uuidString WRITE set_uuidString NOTIFY uuidStringChanged)

public:
	static qPrefUpdateManager *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool dont_check_for_updates() { return prefs.update_manager.dont_check_for_updates; }
	static const QString last_version_used() { return prefs.update_manager.last_version_used; }
	static const QDate next_check() { return QDate::fromJulianDay(prefs.update_manager.next_check); }
	static const QString uuidString() { return st_uuidString; }

public slots:
	static void set_dont_check_for_updates(bool value);
	static void set_last_version_used(const QString& value);
	static void set_next_check(const QDate& value);
	static void set_uuidString(const QString& value);

signals:
	void dont_check_for_updatesChanged(bool value);
	void last_version_usedChanged(const QString& value);
	void next_checkChanged(const QDate& value);
	void uuidStringChanged(const QString& value);

private:
	qPrefUpdateManager() {}

	static void disk_dont_check_for_updates(bool doSync);
	static void disk_last_version_used(bool doSync);
	static void disk_next_check(bool doSync);

	// load only for class variables
	static void load_uuidString();

	// class variables not present in structure preferences
	static QString st_uuidString;
};

#endif
