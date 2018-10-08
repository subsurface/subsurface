// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFCLOUDSTORAGE_H
#define QPREFCLOUDSTORAGE_H
#include "core/pref.h"

#include <QObject>

class qPrefCloudStorage : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool cloud_auto_sync READ cloud_auto_sync WRITE set_cloud_auto_sync NOTIFY cloud_auto_syncChanged);
	Q_PROPERTY(QString cloud_base_url READ cloud_base_url WRITE set_cloud_base_url NOTIFY cloud_base_urlChanged);
	Q_PROPERTY(QString cloud_git_url READ cloud_git_url);
	Q_PROPERTY(QString cloud_storage_email READ cloud_storage_email WRITE set_cloud_storage_email NOTIFY cloud_storage_emailChanged);
	Q_PROPERTY(QString cloud_storage_email_encoded READ cloud_storage_email_encoded WRITE set_cloud_storage_email_encoded NOTIFY cloud_storage_email_encodedChanged);
	Q_PROPERTY(QString cloud_storage_password READ cloud_storage_password WRITE set_cloud_storage_password NOTIFY cloud_storage_passwordChanged);
	Q_PROPERTY(QString cloud_storage_pin READ cloud_storage_pin WRITE set_cloud_storage_pin NOTIFY cloud_storage_pinChanged);
	Q_PROPERTY(int cloud_verification_status READ cloud_verification_status WRITE set_cloud_verification_status NOTIFY cloud_verification_statusChanged);
	Q_PROPERTY(int cloud_timeout READ cloud_timeout WRITE set_cloud_timeout NOTIFY cloud_timeoutChanged);
	Q_PROPERTY(bool save_password_local READ save_password_local WRITE set_save_password_local NOTIFY save_password_localChanged);

public:
	qPrefCloudStorage(QObject *parent = NULL);
	static qPrefCloudStorage *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	enum cloud_status {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};
	Q_ENUM(cloud_status);

	static bool cloud_auto_sync() { return prefs.cloud_auto_sync; }
	static QString cloud_base_url() { return prefs.cloud_base_url; }
	static QString cloud_git_url() { return prefs.cloud_git_url; }
	static QString cloud_storage_email() { return prefs.cloud_storage_email; }
	static QString cloud_storage_email_encoded() { return prefs.cloud_storage_email_encoded; }
	static QString cloud_storage_password() { return prefs.cloud_storage_password; }
	static QString cloud_storage_pin() { return prefs.cloud_storage_pin; }
	static int cloud_timeout() { return prefs.cloud_timeout; }
	static int cloud_verification_status() { return prefs.cloud_verification_status; }
	static bool save_password_local() { return prefs.save_password_local; }

public slots:
	static void set_cloud_auto_sync(bool value);
	static void set_cloud_base_url(const QString &value);
	static void set_cloud_storage_email(const QString &value);
	static void set_cloud_storage_email_encoded(const QString &value);
	static void set_cloud_storage_password(const QString &value);
	static void set_cloud_storage_pin(const QString &value);
	static void set_cloud_timeout(int value);
	static void set_cloud_verification_status(int value);
	static void set_save_password_local(bool value);

signals:
	void cloud_auto_syncChanged(bool value);
	void cloud_base_urlChanged(const QString &value);
	void cloud_storage_emailChanged(const QString &value);
	void cloud_storage_email_encodedChanged(const QString &value);
	void cloud_storage_passwordChanged(const QString &value);
	void cloud_storage_pinChanged(const QString &value);
	void cloud_timeoutChanged(int value);
	void cloud_verification_statusChanged(int value);
	void save_password_localChanged(bool value);

private:
	// functions to load/sync variable with disk
	static void disk_cloud_auto_sync(bool doSync);
	static void disk_cloud_base_url(bool doSync);
	static void disk_cloud_storage_email(bool doSync);
	static void disk_cloud_storage_email_encoded(bool doSync);
	static void disk_cloud_storage_password(bool doSync);
	static void disk_cloud_storage_pin(bool doSync);
	static void disk_cloud_timeout(bool doSync);
	static void disk_cloud_verification_status(bool doSync);
	static void disk_save_password_local(bool doSync);
};

#endif
