// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUPDATEMANAGER_H
#define QPREFUPDATEMANAGER_H
#include "core/pref.h"

#include <QObject>
#include <QDate>

class qPrefUpdateManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dont_check_for_updates READ dont_check_for_updates WRITE set_dont_check_for_updates NOTIFY dont_check_for_updates_changed);
	Q_PROPERTY(bool dont_check_exists READ dont_check_exists WRITE set_dont_check_exists NOTIFY dont_check_exists_changed);
	Q_PROPERTY(const QString last_version_used READ last_version_used WRITE set_last_version_used NOTIFY last_version_used_changed);
	Q_PROPERTY(const QDate next_check READ next_check WRITE set_next_check NOTIFY next_check_changed);

public:
	qPrefUpdateManager(QObject *parent = NULL);
	static qPrefUpdateManager *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	bool dont_check_for_updates() { return prefs.update_manager.dont_check_for_updates; }
	bool dont_check_exists() { return prefs.update_manager.dont_check_exists; }
	const QString last_version_used() { return prefs.update_manager.last_version_used; }
	const QDate next_check() { return QDate::fromString(QString(prefs.update_manager.next_check), "dd/MM/yyyy"); }

public slots:
	void set_dont_check_for_updates(bool value);
	void set_dont_check_exists(bool value);
	void set_last_version_used(const QString& value);
	void set_next_check(const QDate& value);

signals:
	void dont_check_for_updates_changed(bool value);
	void dont_check_exists_changed(bool value);
	void last_version_used_changed(const QString& value);
	void next_check_changed(const QDate& value);

private:
	void disk_dont_check_for_updates(bool doSync);
	void disk_last_version_used(bool doSync);
	void disk_next_check(bool doSync);
};

#endif
