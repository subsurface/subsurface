// SPDX-License-Identifier: GPL-2.0
#include "qPrefCloudStorage.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("CloudStorage");

qPrefCloudStorage *qPrefCloudStorage::instance()
{
	static qPrefCloudStorage *self = new qPrefCloudStorage;
	return self;
}

void qPrefCloudStorage::loadSync(bool doSync)
{
	disk_cloud_auto_sync(doSync);
	disk_cloud_base_url(doSync);
	disk_cloud_storage_email(doSync);
	disk_cloud_storage_email_encoded(doSync);
	disk_cloud_storage_password(doSync);
	disk_cloud_storage_pin(doSync);
	disk_cloud_timeout(doSync);
	disk_cloud_verification_status(doSync);
	disk_save_password_local(doSync);
}

HANDLE_PREFERENCE_BOOL(CloudStorage, "cloud_auto_sync", cloud_auto_sync);

void qPrefCloudStorage::set_cloud_base_url(const QString &value)
{
	if (value != prefs.cloud_base_url) {
		// only free and set if not default
		if (prefs.cloud_base_url != default_prefs.cloud_base_url) {
			qPrefPrivate::copy_txt(&prefs.cloud_base_url, value);
		}

		disk_cloud_base_url(true);
		emit instance()->cloud_base_urlChanged(value);
	}
}
void qPrefCloudStorage::disk_cloud_base_url(bool doSync)
{
	if (doSync) {
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cloud_base_url"), prefs.cloud_base_url, default_prefs.cloud_base_url);
	} else {
		prefs.cloud_base_url = copy_qstring(qPrefPrivate::propValue(keyFromGroupAndName(group, "cloud_base_url"), default_prefs.cloud_base_url).toString());
	}
}

HANDLE_PREFERENCE_TXT(CloudStorage, "email", cloud_storage_email);

HANDLE_PREFERENCE_TXT(CloudStorage, "email_encoded", cloud_storage_email_encoded);

void qPrefCloudStorage::set_cloud_storage_password(const QString &value)
{
	if (value != prefs.cloud_storage_password) {
		qPrefPrivate::copy_txt(&prefs.cloud_storage_password, value);
		disk_cloud_storage_password(true);
		emit instance()->cloud_storage_passwordChanged(value);
	}
}
void qPrefCloudStorage::disk_cloud_storage_password(bool doSync)
{
	if (doSync) {
		if (prefs.save_password_local)
			qPrefPrivate::propSetValue(keyFromGroupAndName(group, "password"), prefs.cloud_storage_password, default_prefs.cloud_storage_password);
	} else {
		prefs.cloud_storage_password = copy_qstring(qPrefPrivate::propValue(keyFromGroupAndName(group, "password"), default_prefs.cloud_storage_password).toString());
	}
}

HANDLE_PREFERENCE_TXT(CloudStorage, "pin", cloud_storage_pin);

HANDLE_PREFERENCE_INT(CloudStorage, "timeout", cloud_timeout);

HANDLE_PREFERENCE_INT(CloudStorage, "cloud_verification_status", cloud_verification_status);

HANDLE_PREFERENCE_BOOL(CloudStorage, "save_password_local", save_password_local);

QString qPrefCloudStorage::diveshare_uid()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "diveshareExport/uid"), "").toString();
}
void qPrefCloudStorage::set_diveshare_uid(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "diveshareExport/uid"), value, "");
	emit instance()->diveshare_uidChanged(value);
}

bool qPrefCloudStorage::diveshare_private()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "diveshareExport/private"), "").toBool();
}
void qPrefCloudStorage::set_diveshare_private(bool value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "diveshareExport/private"), value, false);
	emit instance()->diveshare_privateChanged(value);
}

QString qPrefCloudStorage::divelogde_user()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "divelogde_user"), "").toString();
}
void qPrefCloudStorage::set_divelogde_user(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "divelogde_user"), value, "");
	emit instance()->divelogde_userChanged(value);
}


QString qPrefCloudStorage::divelogde_pass()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "divelogde_pass"), "").toString();
}
void qPrefCloudStorage::set_divelogde_pass(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "divelogde_pass"), value, "");
	emit instance()->divelogde_passChanged(value);
}
