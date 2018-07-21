// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("CloudStorage");

qPrefCloudStorage::qPrefCloudStorage(QObject *parent) : QObject(parent)
{
}
qPrefCloudStorage*qPrefCloudStorage::instance()
{
	static qPrefCloudStorage *self = new qPrefCloudStorage;
	return self;
}

void qPrefCloudStorage::loadSync(bool doSync)
{
	disk_cloud_base_url(doSync);
	disk_cloud_git_url(doSync);
	disk_cloud_storage_email(doSync);
	disk_cloud_storage_email_encoded(doSync);
	disk_cloud_storage_password(doSync);
	disk_cloud_storage_pin(doSync);
	disk_cloud_timeout(doSync);
	disk_cloud_verification_status(doSync);
	disk_git_local_only(doSync);
	disk_save_password_local(doSync);
	disk_save_userid_local(doSync);
	disk_userid(doSync);
}

void qPrefCloudStorage::set_cloud_base_url(const QString& value)
{
	if (value != prefs.cloud_base_url) {
		// only free and set if not default
		if (prefs.cloud_base_url != default_prefs.cloud_base_url) {
			qPrefPrivate::copy_txt(&prefs.cloud_base_url, value);
			qPrefPrivate::copy_txt(&prefs.cloud_git_url, QString(prefs.cloud_base_url) + "/git");
		}

		disk_cloud_base_url(true);
		emit cloud_base_url_changed(value);
	}
}
void qPrefCloudStorage::disk_cloud_base_url(bool doSync)
{
	LOADSYNC_TXT("/cloud_base_url", cloud_base_url);
	LOADSYNC_TXT("/cloud_git_url", cloud_git_url);
}

void qPrefCloudStorage::set_cloud_git_url(const QString& value)
{
	if (value != prefs.cloud_git_url) {
		// only free and set if not default
		if (prefs.cloud_git_url != default_prefs.cloud_git_url) {
			qPrefPrivate::copy_txt(&prefs.cloud_git_url, value);
		}
		disk_cloud_git_url(true);
		emit cloud_git_url_changed(value);
	}
}
DISK_LOADSYNC_TXT(CloudStorage, "/cloud_git_url", cloud_git_url)

HANDLE_PREFERENCE_TXT(CloudStorage, "/email", cloud_storage_email);

HANDLE_PREFERENCE_TXT(CloudStorage, "/email_encoded", cloud_storage_email_encoded);

void qPrefCloudStorage::set_cloud_storage_newpassword(const QString& value)
{
	if (value == prefs.cloud_storage_newpassword)
		return;

	qPrefPrivate::copy_txt(&prefs.cloud_storage_newpassword, value);

	// NOT saved on disk, because it is only temporary
	emit cloud_storage_newpassword_changed(value);
}

void qPrefCloudStorage::set_cloud_storage_password(const QString& value)
{
	if (value != prefs.cloud_storage_password) {
		qPrefPrivate::copy_txt(&prefs.cloud_storage_password, value);
		disk_cloud_storage_password(true);
		emit cloud_storage_password_changed(value);
	}
}
void qPrefCloudStorage::disk_cloud_storage_password(bool doSync)
{
	if (!doSync || prefs.save_password_local)
		LOADSYNC_TXT("/password", cloud_storage_password);
}

HANDLE_PREFERENCE_TXT(CloudStorage, "/pin", cloud_storage_pin);

HANDLE_PREFERENCE_INT(CloudStorage, "/timeout", cloud_timeout);

HANDLE_PREFERENCE_INT(CloudStorage, "/cloud_verification_status", cloud_verification_status);

HANDLE_PREFERENCE_BOOL(CloudStorage, "/git_local_only", git_local_only);

HANDLE_PREFERENCE_BOOL(CloudStorage, "/save_password_local", save_password_local);

HANDLE_PREFERENCE_BOOL(CloudStorage, "/save_userid_local", save_userid_local);

SET_PREFERENCE_TXT(CloudStorage, userid);
void qPrefCloudStorage::disk_userid(bool doSync)
{
	//WARNING: UserId is stored outside of any group, but it belongs to Cloud Storage.
	const QString group = QStringLiteral("");
	LOADSYNC_TXT("subsurface_webservice_uid", userid);
}
