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
	if (value.toStdString() != prefs.cloud_base_url) {
		// only free and set if not default
		if (prefs.cloud_base_url != default_prefs.cloud_base_url)
			prefs.cloud_base_url = value.toStdString();

		disk_cloud_base_url(true);
		emit instance()->cloud_base_urlChanged(value);
	}
}

void qPrefCloudStorage::store_cloud_base_url(const QString &value)
{
	// this is used if we want to update the on-disk settings without changing
	// the runtime preference
	qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cloud_base_url"), value, QString::fromStdString(default_prefs.cloud_base_url));
}
void qPrefCloudStorage::disk_cloud_base_url(bool doSync)
{
	// we don't allow to automatically write back the prefs value for the cloud_base_url.
	// in order to do that you need to use the explicit function above store_cloud_base_url()
	if (!doSync)
		prefs.cloud_base_url = qPrefPrivate::propValue(keyFromGroupAndName(group, "cloud_base_url"),
				QString::fromStdString(default_prefs.cloud_base_url)).toString().toStdString();
}

HANDLE_PREFERENCE_TXT(CloudStorage, "email", cloud_storage_email);

HANDLE_PREFERENCE_TXT(CloudStorage, "email_encoded", cloud_storage_email_encoded);

void qPrefCloudStorage::set_cloud_storage_password(const QString &value)
{
	if (value.toStdString() != prefs.cloud_storage_password) {
		prefs.cloud_storage_password = value.toStdString();
		disk_cloud_storage_password(true);
		emit instance()->cloud_storage_passwordChanged(value);
	}
}
void qPrefCloudStorage::disk_cloud_storage_password(bool doSync)
{
	if (doSync) {
		if (prefs.save_password_local)
			qPrefPrivate::propSetValue(keyFromGroupAndName(group, "password"), prefs.cloud_storage_password,
					default_prefs.cloud_storage_password);
	} else {
		prefs.cloud_storage_password = qPrefPrivate::propValue(keyFromGroupAndName(group, "password"),
				default_prefs.cloud_storage_password).toString().toStdString();
	}
}

HANDLE_PREFERENCE_TXT(CloudStorage, "pin", cloud_storage_pin);

HANDLE_PREFERENCE_INT(CloudStorage, "timeout", cloud_timeout);

HANDLE_PREFERENCE_INT(CloudStorage, "cloud_verification_status", cloud_verification_status);

HANDLE_PREFERENCE_BOOL(CloudStorage, "save_password_local", save_password_local);

QString qPrefCloudStorage::diveshare_uid()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "diveshareExport/uid"), QString()).toString();
}
void qPrefCloudStorage::set_diveshare_uid(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "diveshareExport/uid"), value, QString());
	emit instance()->diveshare_uidChanged(value);
}

bool qPrefCloudStorage::diveshare_private()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "diveshareExport/private"), QString()).toBool();
}
void qPrefCloudStorage::set_diveshare_private(bool value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "diveshareExport/private"), value, false);
	emit instance()->diveshare_privateChanged(value);
}

QString qPrefCloudStorage::divelogde_user()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "divelogde_user"), QString()).toString();
}
void qPrefCloudStorage::set_divelogde_user(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "divelogde_user"), value, QString());
	emit instance()->divelogde_userChanged(value);
}

QString qPrefCloudStorage::divelogde_pass()
{
	return qPrefPrivate::propValue(keyFromGroupAndName("", "divelogde_pass"), QString()).toString();
}
void qPrefCloudStorage::set_divelogde_pass(const QString &value)
{
	qPrefPrivate::propSetValue(keyFromGroupAndName("", "divelogde_pass"), value, QString());
	emit instance()->divelogde_passChanged(value);
}
