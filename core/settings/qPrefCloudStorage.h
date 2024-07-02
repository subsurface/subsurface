// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFCLOUDSTORAGE_H
#define QPREFCLOUDSTORAGE_H
#include "core/pref.h"

#include <QObject>

class qPrefCloudStorage : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool cloud_auto_sync READ cloud_auto_sync WRITE set_cloud_auto_sync NOTIFY cloud_auto_syncChanged)
	Q_PROPERTY(QString cloud_base_url READ cloud_base_url WRITE set_cloud_base_url NOTIFY cloud_base_urlChanged)
	Q_PROPERTY(QString cloud_storage_email READ cloud_storage_email WRITE set_cloud_storage_email NOTIFY cloud_storage_emailChanged)
	Q_PROPERTY(QString cloud_storage_email_encoded READ cloud_storage_email_encoded WRITE set_cloud_storage_email_encoded NOTIFY cloud_storage_email_encodedChanged)
	Q_PROPERTY(QString cloud_storage_password READ cloud_storage_password WRITE set_cloud_storage_password NOTIFY cloud_storage_passwordChanged)
	Q_PROPERTY(QString cloud_storage_pin READ cloud_storage_pin WRITE set_cloud_storage_pin NOTIFY cloud_storage_pinChanged)
	Q_PROPERTY(int cloud_verification_status READ cloud_verification_status WRITE set_cloud_verification_status NOTIFY cloud_verification_statusChanged)
	Q_PROPERTY(int cloud_timeout READ cloud_timeout WRITE set_cloud_timeout NOTIFY cloud_timeoutChanged)
	Q_PROPERTY(bool save_password_local READ save_password_local WRITE set_save_password_local NOTIFY save_password_localChanged)
	Q_PROPERTY(QString diveshare_uid READ diveshare_uid WRITE set_diveshare_uid NOTIFY diveshare_uidChanged);
	Q_PROPERTY(bool diveshare_private READ diveshare_private WRITE set_diveshare_private NOTIFY diveshare_privateChanged);
	Q_PROPERTY(QString divelogde_user READ divelogde_user WRITE set_divelogde_user NOTIFY divelogde_userChanged);
	Q_PROPERTY(QString divelogde_pass READ divelogde_pass WRITE set_divelogde_pass NOTIFY divelogde_passChanged);

public:
	static qPrefCloudStorage *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	// NOTE: these enums are duplicated in mobile-widgets/qmlinterface.h
	enum cloud_status {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};
	Q_ENUM(cloud_status);

	static bool cloud_auto_sync() { return prefs.cloud_auto_sync; }
	static QString cloud_base_url() { return QString::fromStdString(prefs.cloud_base_url); }
	static QString cloud_storage_email() { return QString::fromStdString(prefs.cloud_storage_email); }
	static QString cloud_storage_email_encoded() { return QString::fromStdString(prefs.cloud_storage_email_encoded); }
	static QString cloud_storage_password() { return QString::fromStdString(prefs.cloud_storage_password); }
	static QString cloud_storage_pin() { return QString::fromStdString(prefs.cloud_storage_pin); }
	static int cloud_timeout() { return prefs.cloud_timeout; }
	static int cloud_verification_status() { return prefs.cloud_verification_status; }
	static bool save_password_local() { return prefs.save_password_local; }
	static QString diveshare_uid();
	static bool diveshare_private();
	static QString divelogde_user();
	static QString divelogde_pass();

public slots:
	static void set_cloud_auto_sync(bool value);
	static void set_cloud_base_url(const QString &value);
	static void store_cloud_base_url(const QString &value);
	static void set_cloud_storage_email(const QString &value);
	static void set_cloud_storage_email_encoded(const QString &value);
	static void set_cloud_storage_password(const QString &value);
	static void set_cloud_storage_pin(const QString &value);
	static void set_cloud_timeout(int value);
	static void set_cloud_verification_status(int value);
	static void set_save_password_local(bool value);
	static void set_diveshare_uid(const QString &value);
	static void set_diveshare_private(bool value);
	static void set_divelogde_user(const QString &value);
	static void set_divelogde_pass(const QString &value);

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
	void diveshare_uidChanged(const QString &value);
	void diveshare_privateChanged(bool value);
	void divelogde_userChanged(const QString &value);
	void divelogde_passChanged(const QString &value);

private:
	qPrefCloudStorage() {}

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
