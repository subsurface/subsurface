// SPDX-License-Identifier: GPL-2.0
#include "qPrefCloudStorage.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("CloudStorage");

qPrefCloudStorage::qPrefCloudStorage(QObject *parent) : QObject(parent)
{
}

qPrefCloudStorage *qPrefCloudStorage::instance()
{
	static qPrefCloudStorage *self = new qPrefCloudStorage;
	return self;
}

void qPrefCloudStorage::loadSync(bool doSync)
{
	disk_cloud_base_url(doSync);
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

void qPrefCloudStorage::set_cloud_base_url(const QString &value)
{
	QString valueGit = value + "/git";
	if (value != prefs.cloud_base_url) {
		// only free and set if not default
		if (prefs.cloud_base_url != default_prefs.cloud_base_url) {
			qPrefPrivate::copy_txt(&prefs.cloud_base_url, value);
			qPrefPrivate::copy_txt(&prefs.cloud_git_url, QString(prefs.cloud_base_url) + "/git");
		}

		disk_cloud_base_url(true);
		emit instance()->cloud_base_url_changed(value);
	}
}
void qPrefCloudStorage::disk_cloud_base_url(bool doSync)
{
	if (doSync) {
		qPrefPrivate::propSetValue(group + "/cloud_base_url", prefs.cloud_base_url, default_prefs.cloud_base_url);
	} else {
		prefs.cloud_base_url = copy_qstring(qPrefPrivate::propValue(group + "/cloud_base_url", default_prefs.cloud_base_url).toString());
		qPrefPrivate::copy_txt(&prefs.cloud_git_url, QString(prefs.cloud_base_url) + "/git");
	}
}

HANDLE_PREFERENCE_TXT(CloudStorage, "/email", cloud_storage_email);

HANDLE_PREFERENCE_TXT(CloudStorage, "/email_encoded", cloud_storage_email_encoded);

void qPrefCloudStorage::set_cloud_storage_newpassword(const QString &value)
{
	if (value == prefs.cloud_storage_newpassword)
		return;

	qPrefPrivate::copy_txt(&prefs.cloud_storage_newpassword, value);

	// NOT saved on disk, because it is only temporary
	emit instance()->cloud_storage_newpassword_changed(value);
}

void qPrefCloudStorage::set_cloud_storage_password(const QString &value)
{
	if (value != prefs.cloud_storage_password) {
		qPrefPrivate::copy_txt(&prefs.cloud_storage_password, value);
		disk_cloud_storage_password(true);
		emit instance()->cloud_storage_password_changed(value);
	}
}
void qPrefCloudStorage::disk_cloud_storage_password(bool doSync)
{
	if (doSync) {
		if (prefs.save_password_local)
			qPrefPrivate::propSetValue(group + "/password", prefs.cloud_storage_password, default_prefs.cloud_storage_password);
	} else {
		prefs.cloud_storage_password = copy_qstring(qPrefPrivate::propValue(group + "/password", default_prefs.cloud_storage_password).toString());
	}
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
	if (doSync) {
		// always save in new position (part of cloud storage group)
		qPrefPrivate::propSetValue(group + "/subsurface_webservice_uid", prefs.userid, default_prefs.userid);
	} else {
		//WARNING: UserId was  stored outside of any group.
		// try to read from new location, if it fails read from old location
		prefs.userid = copy_qstring(qPrefPrivate::propValue(group + "/subsurface_webservice_uid", "NoUserIdHere").toString());
		if (QString(prefs.userid) == "NoUserIdHere") {
			const QString group = QStringLiteral("");
			prefs.userid = copy_qstring(qPrefPrivate::propValue(group + "/subsurface_webservice_uid", default_prefs.userid).toString());
		}
	}
}
